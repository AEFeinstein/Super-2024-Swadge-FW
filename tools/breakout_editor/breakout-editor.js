tiled.registerMapFormat("Breakout", {
	name: "Breakout map format",
	extension: "bin",
    read: (fileName) => {
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

        var tileset = tiled.open(filePath + '/breakout-tileset.tsx');
        
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
		for (let i = 0; i < map.layerCount; ++i) {
			const layer = map.layerAt(i);

			if (!layer.isTileLayer) {
				continue;
			}

            let file = new BinaryFile(fileName, BinaryFile.WriteOnly);
            let buffer = new ArrayBuffer(2 + layer.width * layer.height + 2); //Buffer sized to lenth byte + width byte + length * width of level bytes + 2 total target tile bytes
            let view = new Uint8Array(buffer);

            //The first two bytes contain the width and height of the tilemap in tiles
            view[0]=layer.width;
            view[1]=layer.height;
            let writePosition = 2;
            let totalTargetBlockTiles = 0;

			for (let y = 0; y < layer.height; ++y) {
				const row = [];

				for (let x = 0; x < layer.width; ++x) {
					const tile = layer.tileAt(x, y);
                    if(!tile){
                        //file.write(0);
                        view[writePosition] = 0;
                        writePosition++;
                        continue;
                    }

                    const tileId = tile.id;
   
                    //Handle "target block tiles"
                    //These are the blocks that the player must break to complete the level.
                    if(tileId >= 16 && tileId <= 127) {
                        totalTargetBlockTiles++;
                    }

                    //Handling every tile
                    view[writePosition]=tileId;

                    writePosition++;
				}				
			}

            //The last 2 bytes hold the total number of "target block tiles"
            //Forced into a (hopefully) unsigned 16 bit integer, little endian
            //There's probably a better way to do this...
            let totalTargetBlockTilesLowerByte = totalTargetBlockTiles & 255;
            let totalTargetBlockTilesUpperByte = (totalTargetBlockTiles >> 8) & 255;
            view[writePosition] = totalTargetBlockTilesLowerByte;
            writePosition++;
            view[writePosition] = totalTargetBlockTilesUpperByte;
            writePosition++;

            file.write(buffer);
            file.commit();
		}
	},
});