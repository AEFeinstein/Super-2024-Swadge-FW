tiled.registerMapFormat("Mega Pulse EX", {
	name: "Mega Pulse EX map format",
	extension: "bin",
    /*read: (fileName) => {
        var file = new BinaryFile(fileName, BinaryFile.ReadOnly);
        var filePath = FileInfo.path(fileName);
        var buffer = file.read(file.size);
        var view = new Uint8Array(buffer);
        const tileDataOffset = 2;
        const tileSizeInPixels = 16;

        var map = new TileMap();

        //The first two bytes contain the width and height of the tilemap in tiles
        var tileDataLength = view[0] * view[1];
        map.setSize(view[0], view[1]);
        map.setTileSize(tileSizeInPixels, tileSizeInPixels);

        var tileset = tiled.open(filePath + '/swadge-land-tileset.tsx');
        
        var layer = new TileLayer();

        map.addTileset(tileset);

        layer.width = map.width;
        layer.height = map.height;
        layer.name = 'Main';

        var layerEdit = layer.edit();
        var importTileX = 0;
        var importTileY = 0;

        
        //Import tile data
        for(let i = 0; i < tileDataLength; i++){
            let tileId = view[i + tileDataOffset];
            layerEdit.setTile(importTileX, importTileY, tileset.tile(tileId));

            importTileX++;
            if(importTileX >= map.width){
                importTileY++;
                importTileX=0;
            }
        }

        //Reconstruct warp tiles
        var warpDataOffset = tileDataLength + tileDataOffset;
        const warpTileIdOffset = 1;
        for(let i = 0; i < 32; i+=2){
            let warpX = view[warpDataOffset + i];
            let warpY = view[warpDataOffset + i + 1];
            if(warpX != 0 || warpY != 0){
                layerEdit.setTile(warpX, warpY, tileset.tile( (i + warpTileIdOffset + 1) >> 1));
            }
        }

        layerEdit.apply();
        
        map.addLayer(layer);
        file.close();
        return map;
    },*/
	write: (map, fileName) => {
        const tileSizeInPowersOf2 = 4;

        var outputBuffer = [];
        //tiled.log(outputBuffer);
        var tilemapBuffer = [];
        //var warpBuffer = [];
        var entitiesBuffer = [];

        /*var warps = [];
        for(let i = 0; i < 16; i++){
            warps[i] = {x: 0, y: 0};
        }*/

    

        //Handle tiles
		for (let i = 0; i < map.layerCount; ++i) {
			var layer = map.layerAt(i);

            // Only one tile layer is supported.
            // It must be named "tiles".
            // Other tile layers will be ignored

			switch(layer.name) {
                case "tiles":
                    if(!layer.isTileLayer){
                        continue;
                    }

                    //The first two bytes contain the width and height of the tilemap in tiles
                    tilemapBuffer.push(layer.width);
                    tilemapBuffer.push(layer.height);

                    for (let y = 0; y < layer.height; ++y) {

                        for (let x = 0; x < layer.width; ++x) {
                            const tile = layer.tileAt(x, y);
                            if(!tile){
                                tilemapBuffer.push(0);
                                continue;
                            }

                            const tileId = tile.id;

                            if(y < layer.height -1 && tileId > 0 && tileId < 17){
                                //Handling warp tiles
                                const tileIdBelowCurrentTile = layer.tileAt(x,y+1).id;

                                if(tileIdBelowCurrentTile == 34 || tileIdBelowCurrentTile == 64 || tileIdBelowCurrentTile == 158){
                                    //if tile below warp tile is brick block or container or checkpoint, write it like normal
                                    tilemapBuffer.push(tileId);
                                } else {
                                    //otherwise store it in warps array and don't write it into the file just yet
                                    warps[tileId-1].x = x;
                                    warps[tileId-1].y = y;
                                    tilemapBuffer.push(0);
                                }
                            } else {
                                //Handling every other tile
                                tilemapBuffer.push(tileId);
                            }
                        }				
                    }
                    break;
                case "entities":
                    if(!layer.isObjectLayer){
                        continue;
                    }

                    entitiesBuffer.push(layer.objectCount & 0b11111111);
                    entitiesBuffer.push( (layer.objectCount & 0b1111111100000000) >> 8);
                    
                    for (let i= 0; i < layer.objectCount; ++i) {
                        const entity = layer.objects[i];
                        var linkedEntityIndex = (entity.resolvedProperty("linkedEntitySpawn") != null) ? (layer.objects.findIndex( ((item) => item.id == entity.resolvedProperty("linkedEntitySpawn").id))) : 65535;
                        tiled.log(linkedEntityIndex);

                        entitiesBuffer.push(entity.resolvedProperty("type").value);
                        entitiesBuffer.push(Math.floor(entity.x) >> tileSizeInPowersOf2);
                        entitiesBuffer.push(Math.floor(entity.y) >> tileSizeInPowersOf2);
                        entitiesBuffer.push(Math.floor(entity.x) % 16);
                        entitiesBuffer.push(Math.floor(entity.y) % 16);
                        entitiesBuffer.push((entity.tileFlippedVertically ? 2 : 0) + (entity.tileFlippedHorizontally ? 1 : 0));
                        entitiesBuffer.push((entity.resolvedProperty("special0") != null) ? Math.floor(entity.resolvedProperty("special0")) : 0);
                        entitiesBuffer.push((entity.resolvedProperty("special1") != null) ? Math.floor(entity.resolvedProperty("special1")) : 0);
                        entitiesBuffer.push((entity.resolvedProperty("special2") != null) ? Math.floor(entity.resolvedProperty("special2")) : 0);
                        entitiesBuffer.push((entity.resolvedProperty("special3") != null) ? Math.floor(entity.resolvedProperty("special3")) : 0);
                        entitiesBuffer.push((entity.resolvedProperty("special4") != null) ? Math.floor(entity.resolvedProperty("special4")) : 0);
                        entitiesBuffer.push((entity.resolvedProperty("special5") != null) ? Math.floor(entity.resolvedProperty("special5")) : 0);
                        entitiesBuffer.push((entity.resolvedProperty("special6") != null) ? Math.floor(entity.resolvedProperty("special6")) : 0);
                        entitiesBuffer.push((entity.resolvedProperty("special7") != null) ? Math.floor(entity.resolvedProperty("special7")) : 0);
                        entitiesBuffer.push(linkedEntityIndex & 0b11111111);
                        entitiesBuffer.push((linkedEntityIndex & 0b1111111100000000) >> 8);
                    }

                    break;
                default:
                    tiled.log(layer.name);
                    tiled.log(typeof outputBuffer);
                    //outputBuffer.push(255);
                    tiled.log("hi");
                    break;
            }

            

            //Warp tiles are deprecated but maintained 
            //for backwards compatibility with Swadge Land format.
            /*for(let i = 0; i < 16; i++){
                warpBuffer.push(warps[i].x);
                warpBuffer.push(warps[i].y);
            }*/

            outputBuffer = tilemapBuffer/*.concat(warpBuffer)*/.concat(entitiesBuffer);
            //tiled.log(outputBuffer);
            
            let file = new BinaryFile(fileName, BinaryFile.WriteOnly);
            let buffer = new ArrayBuffer(outputBuffer.length); //Buffer sized to max width of level
            let view = new Uint8Array(buffer);
            view.set(outputBuffer);

            file.write(buffer);
            file.commit();
		}
	},
});