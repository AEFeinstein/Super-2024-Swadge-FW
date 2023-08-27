var customMapFormat = {
    name: "Swadge Sokobon Level Format",
    extension: "bin",
    write:

    function(p_map,p_fileName) {

        //Special Characters
        var sokoSigs =
        {
            stackInPlace: 201,
            compress: 202,
            player: 203,
            crate: 204,
            warpinternal: 205,
            warpinternalexit: 206,
            warpexternal: 207,
            button: 208,
            laserEmitUp: 209,
            laserReceiveOmni: 210,
            laserReceiveUp: 211,
            laser90Right: 212,
            ghostblock: 213,
            stackObjEnd: 230
        }

        var m = {
            width: p_map.width,
            height: p_map.height,
            layers: []
        };
        
        var sokoTileLayer, sokoObjectLayer;
        var objArr = [];
        //tiled.time("Export completed in");
        for (var i = 0; i < p_map.layerCount; ++i)
        {
            var layer = p_map.layerAt(i);
            if(layer.isTileLayer)
            {
                sokoTileLayer = layer;
                tiled.log("Layer " + i + " is Tile Layer");
            }
            if(layer.isObjectLayer)
            {
                sokoObjectLayer = layer;
                tiled.log("Layer " + i + " is Object Layer");
            }
        }
        sokoObjectLayer.objects.forEach( function(arrItem, ind)
        {
            tiled.log(ind);
            
            tiled.log(arrItem.tile.className);
            var xval = Math.round(arrItem.x / arrItem.width);
            var yval = Math.round(arrItem.y / arrItem.height - 1);
            var posit = xval + yval * sokoTileLayer.width;
            
            tiled.log("(" + xval + "," + yval + ") Pos: " + posit + "(Width: " + sokoTileLayer.width + ")");
            var props = arrItem.resolvedProperties();
            tiled.log(JSON.stringify(props))
            tiled.log("-------------");
            var objItem =
            {
                obj: arrItem,
                pos: posit,
                x: xval,
                y: yval,
                index: ind
            };
            objArr.push(objItem);
        }

        

        )
        objArr.sort((a,b)=>(a.pos > b.pos) ? -1 : 1); //Sort by index in descending order so we can just split and insert stacked objects
        objArr.forEach( function(arrItem)
            {
                tiled.log("Index:" + arrItem.index + ";Pos:" + arrItem.pos + ":(" + arrItem.x + ","+arrItem.y + ")");
            }
        )
        for (var i = 0; i < p_map.layerCount; ++i)
        {
            var layer = p_map.layerAt(i);
            
            if(!layer.isTileLayer)
            {
                continue;
            }

            data = [];
            if(layer.isTileLayer) {
                data.push(layer.width);
                data.push(layer.height);
                var rows = [];
                for (y = 0; y < layer.height; ++y) {
                    var row = [];
                    for (x = 0; x < layer.width; ++x)
                    {
                        row.push(layer.cellAt(x,y).tileId);
                        data.push(layer.cellAt(x,y).tileId+1);
                    }
                    rows.push(row);
                }   

                //PackInObjects
                if(1)
                {
                objArr.forEach(function(objItem, ind, objArr)
                    {
                        headerOffset = 2;
                        var objClassName = objItem.obj.tile.className;
                        tiled.log(sokoSigs[objClassName]);
                        var propertyVals = [111];
                        propertyVals = propExtract(objItem, objArr);
                        insertionData = [sokoSigs.stackInPlace, sokoSigs[objClassName]].concat(propertyVals).concat([sokoSigs.stackObjEnd]);
                        tiled.log("DataBefore: " + data.slice(0,objItem.pos+headerOffset+1));
                        tiled.log("InsertionData: " + insertionData);
                        tiled.log("DataAfter: " + data.slice(objItem.pos+headerOffset+1));
                        data = data.slice(0,objItem.pos+headerOffset+1).concat(insertionData).concat(data.slice(objItem.pos+headerOffset+1));
                    }

                )
                }
                m.layers.push(rows);
                tiled.log(m.layers);
                //var file = new TextFile(fileName, TextFile.WriteOnly);
                tiled.log("Export to " + p_fileName);
                let view = Uint8Array.from(data);
                let fileHand = new BinaryFile(p_fileName, BinaryFile.WriteOnly);
                let buffer = view.buffer.slice(view.byteOffset, view.byteLength + view.byteOffset);
                //let buffer = view.buffer;
                tiled.log(view);
                fileHand.write(buffer);
                fileHand.commit();
                tiled.log(buffer);
            }
        }
        
        
    }


}

function findObjCoordById(objArr,id)
{
    var loopArgs =
    {
        id: id,
        retVal: {
            x: 0,
            y: 0,
            valid: false,
            index: 0
        }
    }
    objArr.forEach( function(objEntry, ind, arr){
        //tiled.log("Target ID: " + this.id + " Entry ID: " + objEntry.obj.id + " Pos:(" + objEntry.x + "," + objEntry.y + ")");
    
        if(this.id == objEntry.obj.id)
        {
            //tiled.log("MATCH!");
            this.retVal = {
                x: objEntry.x,
                y: objEntry.y,
                valid: true,
                index: ind
            };
        }
    
    } , loopArgs
    )
    return loopArgs.retVal;
}

function propExtract(objItem, objArr)
{
    
    soko_direction =
    {
        UP: 0,
        DOWN: 1,
        RIGHT: 2,
        LEFT: 3
    };
    soko_player_gamemodes =
    {
        SOKO_OVERWORLD: 0,
        SOKO_CLASSIC: 1,
        SOKO_EULER: 2,
        SOKO_LASERBOUNCE: 3
    };
    soko_crate_properties =
    {
        sticky: 0b1,
        trail: 0b10
    };
    soko_warpinternal_properties =
    {
        allow_crates: 0b1
    };
    soko_warpexternal_properties =
    {
        manualIndex: 0b1
    };
    soko_laser90Right_properties =
    {
        emitDirection: 0b1,
        playerMove: 0b10
    };
    soko_laserEmitUp_properties =
    {
        playerMove: 0b10
    };
    soko_button_properties =
    {
        cratePress: 0b1,
        playerPress: 0b10,
        invertAction: 0b100,
        stayDownOnPress: 0b1000
    };
    soko_ghostblock_properties =
    {
        inverted: 0b100,
        playerMove: 0b10
    };
    
    var properties = objItem.obj.resolvedProperties();
    
    retVal = [];

    switch(objItem.obj.tile.className)
    {
        case "player":
            retVal.push(soko_player_gamemodes[properties.gamemode]);
            break;
        case "crate":
            var variant = 0b0;
            if(properties.sticky)
            {
                variant = variant | soko_crate_properties.sticky;
            }
            if(properties.trail)
            {
                variant = variant | soko_crate_properties.trail;
            }
            retVal.push(variant);
            break;
        case "laser90Right":
            var variant = 0b0;
            if(properties.emitDirection)
            {
                variant = variant | soko_laser90Right_properties.emitDirection;
                //tiled.log("laser90Right:emitDirection:" + properties.emitDirection);
            }
            if(properties.playerMove)
            {
                variant = variant | soko_laser90Right_properties.playerMove;
                //tiled.log("laser90Right:emitDirection:" + properties.playerMove);
            }
            retVal.push(variant);
            break;
        case "laserEmitUp":
            var variant = 0b0;
            if(properties.playerMove)
            {
                variant = variant | soko_laserEmitUp_properties.playerMove;
            }
            //tiled.log("" + properties.emitDirection + " " + soko_direction[properties.emitDirection]);
            variant = variant | ((soko_direction[properties.emitDirection] & 0b11) << 6);
            retVal.push(variant);
            break;
        case "laserReceiveUp":
            var variant = 0b0;
            tiled.log("" + properties.emitDirection + " " + soko_direction[properties.emitDirection]);
            variant = variant | ((soko_direction[properties.emitDirection] & 0b11) << 6);
            retVal.push(variant);
            break
        case "warpinternal":
            var variant = 0b0;
            if(properties.allow_crates)
            {
                variant = variant | soko_warpinternal_properties.allow_crates;
                //tiled.log("laser90Right:emitDirection:" + properties.emitDirection);
            }
            retVal.push(variant);
            retVal.push(properties.hp);
            var targetCoord = findObjCoordById(objArr,properties.target_id);
            //if(targetCoord.valid)
            //{
                //tiled.log("className === warpinternalexit || className === warpinternal: " + ((objArr[targetCoord.index].obj.tile.className === "warpinternalexit") || (objArr[targetCoord.index].obj.tile.className === "warpinternal")));
            //}
            if(!targetCoord.valid)
            {
                tiled.log("No Valid Warp Exit at target_id");
            }
            if(targetCoord.valid && ((objArr[targetCoord.index].obj.tile.className === "warpinternalexit") || (objArr[targetCoord.index].obj.tile.className === "warpinternal"))){
                tiled.log("Warp Valid ID: " + properties.target_id + " at Coord(" + targetCoord.x + "," + targetCoord.y + ")");
                //retVal.push(properties[idString]);
                retVal.push(targetCoord.x);
                retVal.push(targetCoord.y);
            }
            break;
        case "warpexternal":
            var variant = 0b0;
            if(properties.manualIndex)
            {
                variant = variant | soko_warpexternal_properties.manualIndex;
                //tiled.log("laser90Right:emitDirection:" + properties.emitDirection);
            }
            retVal.push(variant);
            retVal.push(target_id);
            break;
        case "button":
            var variant = 0b0;
            if(properties.cratePress)
            {
                variant = variant | soko_button_properties.cratePress;
            }
            if(properties.invertAction)
            {
                variant = variant | soko_button_properties.invertAction;
            }
            if(properties.playerPress)
            {
                variant = variant | soko_button_properties.playerPress;
            }
            if(properties.stayDownOnPress)
            {
                variant = variant | soko_button_properties.stayDownOnPress;
            }
            var numTarg = (properties.numTargets & 0b111);
            variant = variant | (numTarg << 5); //store the number of targets in the upper 5 bits (up to 7 targets per button)
            retVal.push(variant);
            for(var i = 0; i < numTarg; ++i)
            {
                idString = "target" + (i+1) + "id";
                var targetCoord = findObjCoordById(objArr,properties[idString]);
                if(targetCoord.valid){
                    tiled.log("Valid ID:" + properties[idString] + " at Coord(" + targetCoord.x + "," + targetCoord.y + ")");
                    //retVal.push(properties[idString]);
                    retVal.push(targetCoord.x);
                    retVal.push(targetCoord.y);
                }
                else
                {
                    numTarg -= 1; //discard invalid target ID, reduce target count by 1
                    variant = variant & 0b11111;
                    variant = variant | (numTarg << 5);
                    retVal[0] = variant;
                }
            }
            break;
        case "ghostblock":
            var variant = 0b0;
            if(properties.inverted)
            {
                variant = variant | soko_ghostblock_properties.inverted;
            }
            if(properties.playerMove)
            {
                variant = variant | soko_ghostblock_properties.playerMove;
            }
        
    }
    return retVal;
}

tiled.log("Registering Soko Map Export");
//tiled.log(tiled.activeAsset.layers[0].cellAt(3,1));
//map = tiled.activeAsset;
//dat = [];
//dat.push(map.width);
//dat.push(map.height);
//for (var y = 0; y < map.height; ++y)
/*
{
    for(var x = 0; x<map.width; ++x)
    {
        tileHere = map.layers[0].cellAt(x,y).tileId + 1;
        //tiled.log("" + x + "," + y + ":" + tileHere);
        dat.push(tileHere);
    }
}
*/
//tiled.log(dat);
//let view = new Uint8Array(dat);
tiled.registerMapFormat("sokoMap", customMapFormat);