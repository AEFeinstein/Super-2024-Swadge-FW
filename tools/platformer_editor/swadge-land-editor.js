tiled.registerMapFormat("Swadge Land", {
	name: "Super Swadge Land map format",
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
    },
	write: (map, fileName) => {
        var warps = [];
        for(let i = 0; i < 16; i++){
            warps[i] = {x: 0, y: 0};
        }

		for (let i = 0; i < map.layerCount; ++i) {
			const layer = map.layerAt(i);

			if (!layer.isTileLayer) {
				continue;
			}

            let file = new BinaryFile(fileName, BinaryFile.WriteOnly);
            let buffer = new ArrayBuffer(2 + layer.width * layer.height + 32); //Buffer sized to max width of level
            let view = new Uint8Array(buffer);

            //The first two bytes contain the width and height of the tilemap in tiles
            view[0]=layer.width;
            view[1]=layer.height;
            let writePosition = 2;

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

                    if(y < layer.height -1 && tileId > 0 && tileId < 17){
                        //Handling warp tiles
                        const tileIdBelowCurrentTile = layer.tileAt(x,y+1).id;

                        if(tileIdBelowCurrentTile == 34 || tileIdBelowCurrentTile == 64 || tileIdBelowCurrentTile == 158){
                            //if tile below warp tile is brick block or container or checkpoint, write it like normal
                            view[writePosition]=tileId;
                        } else {
                            //otherwise store it in warps array and don't write it into the file just yet
                            warps[tileId-1].x = x;
                            warps[tileId-1].y = y;
                            view[writePosition]=0;
                        }
                    } else {
                        //Handling every other tile
                        view[writePosition]=tileId;
                    }

                    writePosition++;
				}				
			}

            for(let i = 0; i < 16; i++){
                view[writePosition] = warps[i].x;
                view[writePosition+1] = warps[i].y;
                writePosition+=2;
            }

            file.write(buffer);
            file.commit();
		}
	},
});