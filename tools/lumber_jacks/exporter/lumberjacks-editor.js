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

        map.setProperty("enemy1", view[1]); 
        map.setProperty("enemy2", view[2]); 
        map.setProperty("enemy3", view[3]); 
        map.setProperty("enemy4", view[4]); 
        map.setProperty("enemy5", view[5]); 
        map.setProperty("enemy6", view[6]); 
        map.setProperty("enemy7", view[7]); 
        map.setProperty("enemy8", view[8]); 
        
        map.setProperty("ghost spawn time", view[9]); //Seriously Troy! Document your code! 
        //map.setProperty("???", view[10]); // - The maniac who knows where you live!
        
        map.setProperty("time", view[11]);   //Time

        var layerEdit = layer.edit();
        var importTileX = 0;
        var importTileY = 0;

        
        //Import tile data
        for(let i = 0; i < tileDataLength; i++){
            let tileId = view[i + 12] - 1;
            
            if (tileId >= 0)
                layerEdit.setTile(importTileX, importTileY, tileset.tile(tileId));
            
            importTileX++;
            if(importTileX >= map.width){
                importTileY++;
                importTileX=0;
            }
        }

        var writePosition = 12 + tileDataLength;
        map.setProperty("spawnx", view[writePosition]);
        map.setProperty("spawny", (view[writePosition + 1]) + (view[writePosition + 2] << 8));
        map.setProperty("switches", view[writePosition+3]); 
        tiled.log(writePosition);
        /*
        //I need to decide where to put the switches. I believe this is where the switches layout was going to be.
        view [writePosition+4] = 8; //WTF Troy?
        view [writePosition+5] = 9; //WTF Troy?
        view [writePosition+6] = 8; //WTF Troy?
        view [writePosition+7] = 4; //WTF Troy?*/ 
    
        //map.setProperty("???", view[writePosition+4]); //Yay more undocumented variables!
        //map.setProperty("???", view[writePosition+5]); //Yay more undocumented variables!
        //map.setProperty("???", view[writePosition+6]); //Yay more undocumented variables!
        //map.setProperty("???", view[writePosition+7]); //Yay more undocumented variables!

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
        let buffer = new ArrayBuffer(20 + layer.height * layer.width);
        let view = new Uint8Array(buffer);
        let writePosition = 12;
        view[0] = layer.height;
        view[1] = map.property("enemy1"); //Enemy 1
        view[2] = map.property("enemy2"); //Enemy 2
        view[3] = map.property("enemy3"); //Enemy 3
        view[4] = map.property("enemy4"); //Enemy 4
        view[5] = map.property("enemy5"); //Enemy 5
        view[6] = map.property("enemy6"); //Enemy 6
        view[7] = map.property("enemy7"); //Enemy 7
        view[8] = map.property("enemy8"); //Enemy 8
        view[9] = map.property("ghost spawn time");    // Only if Enemy 4 is set to 1 or higher
        view[10] = 0;    //
        view[11] = 30;   //Time
        
        //write level
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
                
                view[writePosition] = (tile.id + 1);
                writePosition++;
            }
        }
        
        view [writePosition] = map.property("spawnx") % 240; // Player X starting position Always < 256
        view [writePosition + 1] = map.property("spawny") & 255;
        view [writePosition + 2] = map.property("spawny") >> 8;
        
        //extra data for I don't know what
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