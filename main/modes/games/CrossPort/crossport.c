#include "CrossPort.h"
#include "library.h"
#include "cam.h"
#include "draw.h"
#include "movement.h"
// #include "saveload.h"
#include "generate.h"
#include "models/allMeshes.h"

const char crossportModeName[] = "CrossPort";

static void crossEnterMode(void);
static void crossExitMode(void);
static void crossMainLoop(int64_t elapsedUs);

Camera cam;
int gameStart = 1;

int newChunks = 1;

int renderAmt = 0;
int sortedAmt = 0;
int collider = 0;
RenderBlock* storedBlocks;
RenderBlock* renderingBlocks;
Vect3i* Collidables;

chunkpos currentChunk;
chunkpos lastChunk;
const int renderRadius = 100;
const float camOffset = 0.7f;
int seed = 0;

// int EntCount = 0;
// int rendEntCount = 0;
// Entity* allEntities;
// Entity* renderingEntities;

const Mesh* meshArray[] = { &cube, &cube, &cube };
// const Mesh* entityModels[] = { &player };

swadgeMode_t crossportMode = {
    .modeName =                     crossportModeName,  // Assign the name we created here
    .wifiMode =                     NO_WIFI,            // If we want WiFi
    .overrideUsb =                  false,              // Overrides the default USB behavior.
    .usesAccelerometer =            false,              // If we're using motion controls
    .usesThermometer =              false,              // If we're using the internal thermometer
    .overrideSelectBtn =            false,              // The select/Menu button has a default behavior. If you want to override it, you can set this to true but you'll need to re-implement the behavior.
    .fnEnterMode =                  crossEnterMode,    // The enter mode function
    .fnExitMode =                   crossExitMode,     // The exit mode function
    .fnMainLoop =                   crossMainLoop,     // The loop function
    .fnAudioCallback =              NULL,               // If the mode uses the microphone
    .fnBackgroundDrawCallback =     NULL,               // Draws a section of the display
    .fnEspNowRecvCb =               NULL,               // If using Wifi, add the receive function here
    .fnEspNowSendCb =               NULL,               // If using Wifi, add the send function here
    .fnAdvancedUSB =                NULL,               // If using advanced USB things.
};

static void makeObjs() {
    const float camX = cam.position.x;
    const float camY = cam.position.y;
    const float camZ = cam.position.z;
    const float renderRadius = 8.0f;
    int facesVisible[6] = {0, 0, 0, 0, 0, 0};
    sortedAmt = 0;

    for (int c = 0; c < CHUNK_AMT; c++) {
        for (int x = 0; x < CHUNK_SIZEX; x++) {
            for (int y = 0; y < CHUNK_SIZEY; y++) {
                for (int z = 0; z < CHUNK_SIZEZ; z++) {
                    int idx = (z * CHUNK_SIZEX * CHUNK_SIZEY) + (y * CHUNK_SIZEX) + x;
                    if (idx < 0 || idx >= (CHUNK_SIZEX * CHUNK_SIZEY * CHUNK_SIZEZ)) { continue; }

                    int block = blocks[c][idx];
                    if (block <= 0) { continue; }

                    checkFaces(x, y, z, c, facesVisible);

                    bool allFacesHidden = true;
                    for (int i = 0; i < 6; i++) {
                        if (facesVisible[i] == 1) {
                            allFacesHidden = false;
                            break;
                        }
                    }
                    if (allFacesHidden) { continue; }

                    float wx = (float)((chunkOffsets[c].x * CHUNK_SIZEX) + x);
                    float wy = (float)((chunkOffsets[c].y * CHUNK_SIZEY) + y);
                    float wz = (float)((chunkOffsets[c].z * CHUNK_SIZEZ) + z);

                    storedBlocks[sortedAmt++] = (RenderBlock){
                        .worldposition = {wx, wy, wz},
                        .face = { facesVisible[0], facesVisible[1], facesVisible[2], facesVisible[3], facesVisible[4], facesVisible[5] },
                        .objType = block-1,
                        .dist = 0.0f,
                        .facingID = 0,
                        .facingDir = { 0.0f, 0.0f, 0.0f },
                        .size = { 1.0f, 1.0f, 1.0f }
                    };
                }
            }
        }
    }
}

static void chunkBoarder(){
    int verts[2][3];
    int points[2][3];

    const int yOffset[2] = {-1, CHUNK_SIZEY+1};
    const int addPoint[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    const float CamXDirSin = -sinf(cam.rotation.x);
    const float CamXDirCos = cosf(cam.rotation.x);
    const float CamYDirSin = -sinf(cam.rotation.y);
    const float CamYDirCos = cosf(cam.rotation.y);
    const float CamZDirSin = -sinf(cam.rotation.z);
    const float CamZDirCos = cosf(cam.rotation.z);

    for (int amt=0; amt < CHUNK_AMT; amt++){
        for (int v=0; v < 4; v++){
            int rend = 1;
            for (int c=0; c < 2; c++){
                float rot[3] = {0.0f, 0.0f, 0.0f};
                RotationMatrix(
                    ((chunkOffsets[amt].x + addPoint[v][0]) * CHUNK_SIZEX) - cam.position.x,
                    ((chunkOffsets[amt].y * CHUNK_SIZEY) + yOffset[c]) - cam.position.y,
                    ((chunkOffsets[amt].z + addPoint[v][1]) * CHUNK_SIZEZ) - cam.position.z,
                    CamYDirSin, CamYDirCos,
                    CamXDirSin, CamXDirCos,
                    CamZDirSin, CamZDirCos,
                    &rot[0]
                );

                if (rot[2] < 0.1f) { rend = 0; break; }
        
                project2D(&points[c][0], rot, cam.fov, cam.nearPlane);
            }

            if (!rend) { continue; }
        
            drawLineFast(points[0][0], points[0][1], points[1][0], points[1][1], c000);
        }
    }
}

static void renderModel(Mesh* model, float wx, float wy, float wz, Vect3f objRot, Vect3f size, int block, int objIndex, int colorDraw, int triDraw, int type){
    const float fovX = 140.0f * (3.14159265f / 180.0f);
    const float fovY = 2.0f * atanf(tanf(fovX * 0.5f) * ((float)(sH) / (float)(sW)));

    const float CamXDirSin = -sinf(cam.rotation.x);
    const float CamXDirCos = cosf(cam.rotation.x);
    const float CamYDirSin = -sinf(cam.rotation.y);
    const float CamYDirCos = cosf(cam.rotation.y);
    const float CamZDirSin = -sinf(cam.rotation.z);
    const float CamZDirCos = cosf(cam.rotation.z);

    int tri1[3][2];
    int tri2[3][2];
    int check[3][2];
    float verts[3][3];
    float clip1[3][3];
    float clip2[3][3];

    int triCount = model->count;

    for (int index=0; index < triCount; index++){
        if (type==1){ if (!renderingBlocks[objIndex].face[index]) { continue; } }
        
        for (int tri=0; tri < 2; tri++){
            for (int v=0; v < 3; v++){
                int triIDX = model->faces[index].tris[tri].p[v];
                float lx = model->verts[triIDX].x - 0.5f;
                float ly = model->verts[triIDX].y - 0.5f;
                float lz = model->verts[triIDX].z - 0.5f;

                float oRot[3];
                RotateVertexObject(lx, ly, lz, objRot.x, objRot.y, objRot.z, size.x, size.y, size.z, &oRot[0]);
                
                RotationMatrix(
                    (wx + (oRot[0] + 0.5f)) - cam.position.x,
                    (wy + (oRot[1] + 0.5f)) - cam.position.y,
                    (wz + (oRot[2] + 0.5f)) - cam.position.z,
                    CamYDirSin, CamYDirCos,
                    CamXDirSin, CamXDirCos,
                    CamZDirSin, CamZDirCos,
                    &verts[v][0]
                );
                
                project2D(&check[v][0], verts[v], cam.fov, cam.nearPlane);
            }

            int output = TriangleClipping(verts, clip1, clip2, fovX, fovY, cam.nearPlane, cam.farPlane);
            if (output <= 0) { continue; }

            if (windingOrder(check[0], check[1], check[2])){
                for (int v=0; v < 3; v++){
                    project2D(&tri1[v][0], clip1[v], cam.fov, cam.nearPlane);
                    if (output == 2){ project2D(&tri2[v][0], clip2[v], cam.fov, cam.nearPlane); }
                }

                if (!windingOrder(tri1[0], tri1[1], tri1[2])) {
                    swapVerts(tri1[1], tri1[2]);
                }
                if (colorDraw) { drawFilledTris(tri1, block, index); }
                if (triDraw) { drawTri(tri1); }

                if (output == 2) {
                    if (!windingOrder(tri2[0], tri2[1], tri2[2])){
                        swapVerts(tri2[1], tri2[2]);
                    }
                    if (colorDraw) { drawFilledTris(tri2, block, index); }
                    if (triDraw) { drawTri(tri2); }
                }
            }
        }
    }
}

static void renderHit(){
    for (int cId=0; cId<2; cId++){
        if ((rayPosition[cId][0] < 0 || rayPosition[cId][0] > CHUNK_SIZEX) || (rayPosition[cId][1] < 0 || rayPosition[cId][1] > CHUNK_SIZEY) || (rayPosition[cId][2] < 0 || rayPosition[cId][2] > CHUNK_SIZEZ)) { continue; }

        int chunk = rayPosition[cId][3];
        int wx    = rayPosition[cId][0] + (chunkOffsets[chunk].x * CHUNK_SIZEX);
        int wy    = rayPosition[cId][1] + (chunkOffsets[chunk].y * CHUNK_SIZEY);
        int wz    = rayPosition[cId][2] + (chunkOffsets[chunk].z * CHUNK_SIZEZ);
        int block = 0;

        Vect3f objRot = { 0.0f, 0.0f, 0.0f };
        Vect3f size = { 1.0f, 1.0f, 1.0f };

        renderModel(meshArray[block], wx, wy, wz, objRot, size, block, cId, 0, 1, 0);
    }
}

static void render() {
    int rendStart = 0;
    int rendLimit = 200;
    if (renderAmt >= rendLimit) { rendStart = renderAmt - rendLimit; }

    // for (int z=0; z < rendEntCount; z++){ renderingEntities[z].rendered = 0; }

    for (int i=rendStart; i < renderAmt; i++){
        int block = renderingBlocks[i].objType;
        if (block < 0 || block >= (sizeof(meshArray)/sizeof(meshArray[0]))) { continue; }

        float wx = renderingBlocks[i].worldposition.x;
        float wy = renderingBlocks[i].worldposition.y;
        float wz = renderingBlocks[i].worldposition.z;
        // float blockDist = renderingBlocks[i].dist;

        Vect3f objRot = { renderingBlocks[i].facingDir.x, renderingBlocks[i].facingDir.y, renderingBlocks[i].facingDir.z };
        Vect3f size = { renderingBlocks[i].size.x, renderingBlocks[i].size.y, renderingBlocks[i].size.z };

        /* for (int z=0; z < rendEntCount; z++){
            if (renderingEntities[z].rendered) { continue; }

            float entDist = renderingEntities[z].dist;
            if (entDist <= blockDist) { continue; }

            int emodel = renderingEntities[z].model;
            if (emodel < 0 || emodel >= (sizeof(entityModels)/sizeof(entityModels[0]))) { continue; }

            float ex = renderingEntities[z].position.x;
            float ey = renderingEntities[z].position.y;
            float ez = renderingEntities[z].position.z;
            Vect3f entRot = { renderingEntities[z].rotation.x, renderingEntities[z].rotation.y, renderingEntities[z].rotation.z };
            Vect3f entSize = { renderingEntities[z].size.x, renderingEntities[z].size.y, renderingEntities[z].size.z };

            renderModel(entityModels[emodel], ex, ey, ez, entRot, entSize, z, 0, 1, 1, 0);
            renderingEntities[z].rendered = 1;
        } */

        renderModel(meshArray[block], wx, wy, wz, objRot, size, block, i, 1, 1, 1);
    }

    /* for (int z=0; z < rendEntCount; z++){
        if (renderingEntities[z].rendered) { continue; }

        int emodel = renderingEntities[z].model;
        if (emodel < 0 || emodel >= (sizeof(entityModels)/sizeof(entityModels[0]))) { continue; }

        float ex = renderingEntities[z].position.x;
        float ey = renderingEntities[z].position.y;
        float ez = renderingEntities[z].position.z;
        Vect3f entRot = { renderingEntities[z].rotation.x, renderingEntities[z].rotation.y, renderingEntities[z].rotation.z };
        Vect3f entSize = { renderingEntities[z].size.x, renderingEntities[z].size.y, renderingEntities[z].size.z };

        renderModel(entityModels[emodel], ex, ey, ez, entRot, entSize, z, 0, 1, 1, 0);
        renderingEntities[z].rendered = 1;
    } */

    chunkBoarder();
    renderHit();

    cam.position.y -= camOffset;
}

static int compareRenderBlock(const void* a, const void* b) {
    float d1 = ((RenderBlock*)a)->dist;
    float d2 = ((RenderBlock*)b)->dist;
    return (d1 < d2) - (d1 > d2);
}

static void sortObjs() {
    renderAmt = 0;
    collider = 0;
    // rendEntCount = 0;

    Vect3f dirNorm;
    Vect3f camForward;
    camForward.x = cosf(cam.rotation.x) * sinf(cam.rotation.y);
    camForward.y = sinf(cam.rotation.x);
    camForward.z = cosf(cam.rotation.x) * cosf(cam.rotation.y);

    Vect3i tilePos;
    tilePos.x = (int)floorf(cam.position.x);
    tilePos.y = (int)floorf(cam.position.y);
    tilePos.z = (int)floorf(cam.position.z);

    for (int i = 0; i < sortedAmt; i++) {
        Vect3f dir;
        dir.x = storedBlocks[i].worldposition.x - cam.position.x + 0.00001f;
        dir.y = storedBlocks[i].worldposition.y - cam.position.y + 0.00001f;
        dir.z = storedBlocks[i].worldposition.z - cam.position.z + 0.00001f;
        float dist = (dir.x*dir.x) + (dir.y*dir.y) + (dir.z*dir.z);
        
        if (dist < (renderRadius * renderRadius) && renderAmt < MAX_OBJECTS_RENDERED) {
            float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);

            if (len > 0.0001f) { 
                dirNorm.x = dir.x/len;
                dirNorm.y = dir.y/len;
                dirNorm.z = dir.z/len;
            } else {
                dirNorm.x = 0.0f;
                dirNorm.y = 0.0f;
                dirNorm.z = 0.0f;
            }

            Vect3i blockTile = { (int)(storedBlocks[i].worldposition.x), (int)(storedBlocks[i].worldposition.y), (int)(storedBlocks[i].worldposition.z)};
            
            for (int z=0; z < 36; z++){
                if (collider >=36) { collider = 36; break; }

                if (tilePos.x + tilePosMove[z].x == blockTile.x &&
                    tilePos.y + tilePosMove[z].y == blockTile.y &&
                    tilePos.z + tilePosMove[z].z == blockTile.z) {
                    Collidables[collider++] = blockTile;
                    break;
                }
            }

            if (dot(dirNorm, camForward) > -0.867f) {
                renderingBlocks[renderAmt++] = (RenderBlock){
                    .worldposition = {storedBlocks[i].worldposition.x, storedBlocks[i].worldposition.y, storedBlocks[i].worldposition.z},
                    .face = { storedBlocks[i].face[0], storedBlocks[i].face[1], storedBlocks[i].face[2], storedBlocks[i].face[3], storedBlocks[i].face[4], storedBlocks[i].face[5] },
                    .objType = storedBlocks[i].objType,
                    .dist = dist,
                    .facingID = storedBlocks[i].facingID,
                    .facingDir = storedBlocks[i].facingDir,
                    .size = storedBlocks[i].size
                };
            }
        }
    }

    qsort(renderingBlocks, renderAmt, sizeof(RenderBlock), compareRenderBlock);

    /* for (int i = 0; i < EntCount; i++) {
        Vect3f dir;
        dir.x = allEntities[i].position.x - cam.position.x + 0.00001f;
        dir.y = allEntities[i].position.y - cam.position.y + 0.00001f;
        dir.z = allEntities[i].position.z - cam.position.z + 0.00001f;
        float dist = (dir.x*dir.x) + (dir.y*dir.y) + (dir.z*dir.z);

        if (dist < (renderRadius * renderRadius)) {
            rendEntCount++;
            renderingEntities = heap_caps_calloc(renderingEntities, rendEntCount * sizeof(Entity), MALLOC_CAP_SPIRAM);
            
            renderingEntities[rendEntCount - 1] = (Entity){
                .position = { allEntities[i].position.x, allEntities[i].position.y, allEntities[i].position.z },
                .rotation = { allEntities[i].rotation.x, allEntities[i].rotation.y, allEntities[i].rotation.z },
                .size = { allEntities[i].size.x, allEntities[i].size.y, allEntities[i].size.z },
                .dist = dist,
                .model = allEntities[i].model,
                .rendered = allEntities[i].rendered
            };
        }
    } */
}

static void checkChunk(){
    if (CHUNK_DISTX==0){ currentChunk.x = 0; } else {
        currentChunk.x = (int)(cam.position.x) / CHUNK_SIZEX;
        if (cam.position.x < 0) currentChunk.x--;
    }
    if (CHUNK_DISTY==0){ currentChunk.y = 0; } else {
        currentChunk.y = (int)(cam.position.y) / CHUNK_SIZEY;
        if (cam.position.y < 0) currentChunk.y--;
    }
    if (CHUNK_DISTZ==0){ currentChunk.z = 0; } else {
        currentChunk.z = (int)(cam.position.z) / CHUNK_SIZEZ;
        if (cam.position.z < 0) currentChunk.z--;
    }

    if (currentChunk.x != lastChunk.x || currentChunk.y != lastChunk.y || currentChunk.z != lastChunk.z){
        int cd = 0;
        for (int cy=(-CHUNK_DISTY); cy <= CHUNK_DISTY; cy++){
            for (int cx=(-CHUNK_DISTX); cx <= CHUNK_DISTX; cx++){
                for (int cz=(-CHUNK_DISTZ); cz <= CHUNK_DISTZ; cz++){
                    chunkOffsets[cd].x = currentChunk.x + cx;
                    chunkOffsets[cd].y = currentChunk.y + cy;
                    chunkOffsets[cd].z = currentChunk.z + cz;
                    cd++;
                }
            }
        }

        cleanWorld();
        // for (int c=0; c < CHUNK_AMT; c++){ generateWorld(chunkOffsets[c].x, chunkOffsets[c].y, chunkOffsets[c].z, c, seed); }

        makeObjs();
    }

    lastChunk.x = currentChunk.x;
    lastChunk.y = currentChunk.y;
    lastChunk.z = currentChunk.z;
}

static void crossEnterMode(void) {
    int total = (CHUNK_SIZEX * CHUNK_SIZEY * CHUNK_SIZEZ);
    storedBlocks = heap_caps_calloc(1, sizeof(RenderBlock) * MAX_OBJECTS, MALLOC_CAP_SPIRAM);
    renderingBlocks = heap_caps_calloc(1, sizeof(RenderBlock) * MAX_OBJECTS_RENDERED, MALLOC_CAP_SPIRAM);
    Collidables = heap_caps_calloc(1, sizeof(Vect3i) * 36, MALLOC_CAP_SPIRAM);
    for (int c = 0; c < CHUNK_AMT; c++) { blocks[c] = heap_caps_calloc(1, sizeof(int) * total, MALLOC_CAP_SPIRAM); }

    cam = createCamera(0.0f, 0.0f, -50.0f, 0.0f, 0.0f, 0.0f, 90.0f, 0.1f, 100.0f);
    setupTilePos();

    checkChunk();
}

static void crossExitMode(void) {
    heap_caps_free(storedBlocks);
    heap_caps_free(renderingBlocks);
    heap_caps_free(Collidables);
    heap_caps_free(blocks);
}

static void crossMainLoop(int64_t elapsedUs) {
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt)) { btn.input = evt.state; }

    int refresh = 0;
    for (int i=0; i<CHUNK_AMT; i++){
        if (chunkDirty[i]){
            refresh = 1;
            break;
        }
    }
    if (refresh) { makeObjs(); refresh=0; }


    checkChunk();
    movePlayer(&cam, Collidables, 0);

    sortObjs();

    cam.position.y += camOffset;
    raycast(&cam);
    placingBlocks(&cam);

    render();

    char pos[64];
    snprintf(pos, sizeof(pos), "Pos: [ %d | %d | %d]", (int)floorf(cam.position.x), (int)floorf(cam.position.y), (int)floorf(cam.position.z));
    drawText( getSysFont(), c555, pos, 40, 40 );
}
