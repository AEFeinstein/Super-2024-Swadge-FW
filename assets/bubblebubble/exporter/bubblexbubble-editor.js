tiled.registerMapFormat("Bubble X Bubble", {
	name: "Bubble X Bubble map format",
	extension: "bin",
    read: (fileName) => {
        var file = new BinaryFile(fileName, BinaryFile.ReadOnly);
        var filePath = FileInfo.path(fileName);
        var buffer = file.read(file.size);
        var view = new Uint8Array(buffer);
        const tileDataOffset = 20;


        var map = new TileMap();

        
        var tileDataLength = 8 * 12;
        map.setSize(8, 12);
        map.setTileSize(16, 16);
        map.orientation = 4;
        map.hexSideLength = 8;
        map.StaggerX = 8;

        var tileset = tiled.open(filePath + '/../exporter/DefaultBubbles.tsx');
        
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
            let tileId = view[i + 20];

            if (tileId > 0)
                layerEdit.setTile(importTileX, importTileY, tileset.tile(tileId));
            
            importTileX++;
            if(importTileX >= map.width){
                importTileY++;
                importTileX=0;
            }
        }
        

        layerEdit.apply();        
        map.addLayer(layer);

        


        file.close();
        return map;
    },
	write: (map, fileName) => {
        var layer;

        for (let i = 0; i < map.layerCount; ++i)
        {
            layer = map.layerAt(0);
            if (layer.isTileLayer) break;
        }

        if (!layer.isTileLayer)
        {
            return;
        }

        let file = new BinaryFile(fileName, BinaryFile.WriteOnly);
        let buffer = new ArrayBuffer(20 + 96);
        let view = new Uint8Array(buffer);
        let writePosition = 20;
        
        //write level
        for (let y = 0; y < 12; ++y)
        {
            for (let x = 0; x < 8; ++x)
            {
                const tile = layer.tileAt(x, y);
                
                if (!tile){
                    view[writePosition] = 0;
                    writePosition++;
                    continue;
                }
                
                view[writePosition] = (tile.id);
                writePosition++;
            }
        }        

        file.write(buffer);
        file.commit();
        }
	},
);