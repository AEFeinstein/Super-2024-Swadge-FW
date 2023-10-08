tiled.registerMapFormat("Lumber Jacks", {
	name: "Lumber Jacks map format",
	extension: "bin",
    read: (fileName) => {
        var file = new BinaryFile(fileName, BinaryFile.ReadOnly);
        var filePath = FileInfo.path(fileName);
        var buffer = file.read(file.size);
        var view = new Uint8Array(buffer);
        const tileDataOffset = 12;
        const tileSizeInPixels = 16;

        var map = new TileMap();

        //The first two bytes contain the width and height of the tilemap in tiles
        var tileDataLength = 18 * view[0];
        map.setSize(18, view[0]);
        map.setTileSize(tileSizeInPixels, tileSizeInPixels);

        var tileset = tiled.open(filePath + '/lumberjacks.tsx');
        
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

        layerEdit.apply();
        
        map.addLayer(layer);
        file.close();
        return map;
    },
	write: (map, fileName) => {
        let file = new BinaryFile(filename, BinaryFile.WriteOnly);
        let buffer = new ArrayBuffer(20 + layer.height * layer.width);
        let view = new Uint8Array(buffer);

        view[0] = layer.height;
        view[1] = 3; //Enemy 1
        view[2] = 0; //Enemy 2
        view[3] = 0; //Enemy 3
        view[4] = 0; //Enemy 4
        view[5] = 0; //Enemy 5
        view[6] = 0; //Enemy 6
        view[7] = 0; //Enemy 7
        view[8] = 0; //Enemy 8
        view[9] = 0;    //
        view[10] = 0;    //
        view[11] = 30;   //Time

        //write level
        const layer = map.layerAt(0);
        let writePosition = 12;
        for (let y = 0; y <layer.height; ++y)
        {
            for (let x = 0; x < layer.width; ++x)
            {
                const tile = layer.tileAt(x, y);

                if (!tile){
                    view[writePosition] = 0;
                    writePosition++;
                    continue;
                }

                view[writePosition] = tile.id;
                writePosition++;
            }
        }

        view [writePosition] = 94; // Player X starting position Always < 256
        
        //16 bit value for where the player is on the Y can be > 256
        view [writePosition+1] = 30;
        view [writePosition+2] = 0;
        /*

            //The last 2 bytes hold the total number of "target block tiles"
            //Forced into a (hopefully) unsigned 16 bit integer, little endian
            //There's probably a better way to do this...
            let totalTargetBlockTilesLowerByte = totalTargetBlockTiles & 255;
            let totalTargetBlockTilesUpperByte = (totalTargetBlockTiles >> 8) & 255;
            view[writePosition] = totalTargetBlockTilesLowerByte;
            writePosition++;
            view[writePosition] = totalTargetBlockTilesUpperByte;
            writePosition++;
        */


        view [writePosition+3] = 2; //Number of switches 
        view [writePosition+4] = 8; //WTF Troy?
        view [writePosition+5] = 9; //WTF Troy?
        view [writePosition+6] = 8; //WTF Troy?
        view [writePosition+7] = 4; //WTF Troy?
    
        file.write(buffer);
        file.commit();
        }
	},
);