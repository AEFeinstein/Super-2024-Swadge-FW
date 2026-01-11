-- Script to export tilemap data as a binary file.
-- Original script by Zeltrix (https://pastebin.com/mQGiKAgR)
-- Export to binary by JVeg199X
-- Note: This script only works with tilemaps of 255x255 tiles or less

-- DO NOT USE. WIP for original binary
-- Check .asp file and .config file

if TilesetMode == nil then return app.alert "Use Aseprite 1.3" end
local spr = app.activeSprite

if not spr then return end

-- TODO Add Multi-File Selection for multiple levels and config files

local d = Dialog("Export Tilemap as .bin File")
d:label{id="lab1",label="",text="Export Tilemap as .bin File for your own GameEngine"}
 :file{id = "path", label="Export Path", filename="",open=false,filetypes={"bin"}, save=true, focus=true}
 :label{id="lab3", label="",text="Max supported tilemap size: 255x255"}
 :separator{}
 :label{id="lab2", label="",text="In the last row of the tilemap-layer there has to be at least one Tile \"colored\" to fully export the whole Tilemap"}
 :button{id="ok",text="&OK",focus=true}
 :button{text="&Cancel" }
 :show()



--Initialize warp data array
local warps = {}
for i=0, 15 do
  warps[i] = {}
  warps[i][0] = 0;
  warps[i][1] = 0;
end

local data = d.data
if not data.ok then return end
    local lay = app.activeLayer
    if(#data.path<=0)then app.alert("No path selected") end
    if not lay.isTilemap then return app.alert("Layer is not tilemap") end
    pc = app.pixelColor
    mapFile = io.open(data.path,"w")

  for _,c in ipairs(lay.cels) do
    local img = c.image

    --The first two bytes contain the width and height of the tilemap in tiles
    mapFile:write(string.char(img.width))
    mapFile:write(string.char(img.height))

    --The next section of bytes is the tilemap itself
    for p in img:pixels() do
      if(p ~= nil) then
        local tileId = p()

        --if(tileId == 130) then
        --  local d2 = Dialog(tileId)
        --  d2:show()
        --end

        if(tileId > 0 and tileId < 17) then
          --warp tiles

          tileBelowCurrentTile = img:getPixel(p.x, p.y+1)
          if(tileBelowCurrentTile == 34 or tileBelowCurrentTile == 64 or tileBelowCurrentTile == 158) then
            --if tile below warp tile is brick block or container or checkpoint, write it like normal
            mapFile:write(string.char(tileId))
          else
            --otherwise store it in warps array and don't write it into the file just yet
            warps[tileId-1][0] = p.x
            warps[tileId-1][1] = p.y
            mapFile:write(string.char(0))
          end
          
        else
          --every other tile
          mapFile:write(string.char(tileId))
        end

      end
    end

    --The last 32 bytes are warp x and y locations
    for i=0, 15 do
      mapFile:write(string.char(warps[i][0]))
      mapFile:write(string.char(warps[i][1]))
    end
  end

    mapFile:close()
     