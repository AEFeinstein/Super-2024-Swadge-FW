import json
import os
import struct
import tkinter as tk
from tkinter import filedialog, messagebox

APP_DIR = os.path.dirname(os.path.abspath(__file__))

GRID_W = 12
GRID_H = 9
CELL = 48
EDGE = 8

N,E,S,W = 8,4,2,1
GOAL = 32
HOLE = 64
SPECIAL = GOAL | HOLE
NAME_MAX = 16
DIR_NONE = 0
DIR_W = 1
DIR_S = 2
DIR_E = 4
DIR_N = 8
DIR_ALL = DIR_W | DIR_S | DIR_E | DIR_N
LEGACY_BOMBADEETLE = 16
LEGACY_ENEMY = 128

maze=[0]*(GRID_W*GRID_H)
entities=[0]*(GRID_W*GRID_H)
enemies=[0]*(GRID_W*GRID_H)
selected=[0]
current_path=[None]
last_direction=[DIR_W]

def idx(x,y): return y*GRID_W+x

def set_bit(i,bit,on):
    if on: maze[i]|=bit
    else: maze[i]&=~bit

def toggle_wall(x,y,bit):
    i=idx(x,y)
    on=not (maze[i]&bit)
    set_bit(i,bit,on)

    if bit==N and y>0: set_bit(idx(x,y-1),S,on)
    if bit==S and y<GRID_H-1: set_bit(idx(x,y+1),N,on)
    if bit==W and x>0: set_bit(idx(x-1,y),E,on)
    if bit==E and x<GRID_W-1: set_bit(idx(x+1,y),W,on)

    redraw()

def sanitize_maze_cell(value):
    return int(value)&0xFF&~LEGACY_BOMBADEETLE&~LEGACY_ENEMY

def sanitize_maze_specials(value):
    value=sanitize_maze_cell(value)
    walls=value&(N|E|S|W)
    if value&GOAL and value&HOLE:
        return walls|GOAL
    return walls|(value&SPECIAL)

def sanitize_facing(value):
    value=int(value)&0xFF&DIR_ALL
    if not value:
        return 0
    for direction in (DIR_N,DIR_E,DIR_S,DIR_W):
        if value&direction:
            return direction
    return 0

def resolve_loaded_tile(i):
    maze[i]=sanitize_maze_specials(maze[i])
    entities[i]=sanitize_facing(entities[i])
    enemies[i]=sanitize_facing(enemies[i])

    if maze[i]&SPECIAL:
        entities[i]=0
        enemies[i]=0
    elif entities[i] and enemies[i]:
        enemies[i]=0

def sanitize_loaded_map():
    for i in range(GRID_W*GRID_H):
        resolve_loaded_tile(i)

def migrate_legacy_map(cells):
    for i,v in enumerate(cells):
        cell=int(v)&0xFF
        entities[i]=DIR_W if cell&LEGACY_BOMBADEETLE else 0
        enemies[i]=DIR_W if cell&LEGACY_ENEMY else 0
        maze[i]=sanitize_maze_cell(cell)

def update_title():
    name=current_path[0] if current_path[0] else "Untitled"
    root.title("Maze Editor - %s"%name)

root=tk.Tk()
root.title("Maze Editor - Untitled")

menubar=tk.Menu(root)
file_menu=tk.Menu(menubar,tearoff=0)
menubar.add_cascade(label="File",menu=file_menu)
root.config(menu=menubar)

canvas=tk.Canvas(root,width=GRID_W*CELL,height=GRID_H*CELL,bg="white")
canvas.pack()

def facing_line(cx,cy,x0,y0,x1,y1,direction,color):
    if direction==DIR_N:
        canvas.create_line(cx,cy,cx,y0+12,width=2,fill=color)
    elif direction==DIR_E:
        canvas.create_line(cx,cy,x1-12,cy,width=2,fill=color)
    elif direction==DIR_S:
        canvas.create_line(cx,cy,cx,y1-12,width=2,fill=color)
    elif direction==DIR_W:
        canvas.create_line(cx,cy,x0+12,cy,width=2,fill=color)

def draw_bombadeetle(x0,y0,x1,y1,direction):
    cx=(x0+x1)//2
    cy=(y0+y1)//2
    canvas.create_oval(x0+14,y0+14,x1-14,y1-14,
                       outline="#6a1b9a",width=2,fill="#ce93d8")
    facing_line(cx,cy,x0,y0,x1,y1,direction,"#4a148c")

def draw_enemy(x0,y0,x1,y1,direction):
    cx=(x0+x1)//2
    cy=(y0+y1)//2
    canvas.create_oval(x0+14,y0+14,x1-14,y1-14,
                       outline="#b71c1c",width=2,fill="#ef9a9a")
    facing_line(cx,cy,x0,y0,x1,y1,direction,"#7f0000")

def redraw():
    canvas.delete("all")
    for y in range(GRID_H):
        for x in range(GRID_W):
            x0=x*CELL
            y0=y*CELL
            x1=x0+CELL
            y1=y0+CELL

            i=idx(x,y)
            fill="#cce5ff" if i==selected[0] else "white"
            canvas.create_rectangle(x0,y0,x1,y1,fill=fill,outline="#ccc")

            bits=maze[i]
            if entities[i]&DIR_ALL:
                draw_bombadeetle(x0,y0,x1,y1,entities[i]&DIR_ALL)
            if enemies[i]&DIR_ALL:
                draw_enemy(x0,y0,x1,y1,enemies[i]&DIR_ALL)
            if bits&GOAL:
                canvas.create_oval(x0+10,y0+10,x1-10,y1-10,
                                   outline="#2e7d32",width=2,fill="#a5d6a7")
            if bits&HOLE:
                canvas.create_oval(x0+12,y0+12,x1-12,y1-12,
                                   outline="#212121",width=2,fill="#424242")
            if bits&N: canvas.create_line(x0,y0,x1,y0,width=3)
            if bits&E: canvas.create_line(x1,y0,x1,y1,width=3)
            if bits&S: canvas.create_line(x0,y1,x1,y1,width=3)
            if bits&W: canvas.create_line(x0,y0,x0,y1,width=3)

def click(event):
    canvas.focus_set()
    x=event.x//CELL
    y=event.y//CELL
    if not (0<=x<GRID_W and 0<=y<GRID_H):
        return

    lx=event.x % CELL
    ly=event.y % CELL

    d={
        W: lx,
        E: CELL-lx,
        N: ly,
        S: CELL-ly
    }

    bit=min(d,key=d.get)
    if d[bit] <= EDGE:
        toggle_wall(x,y,bit)
    else:
        selected[0]=idx(x,y)
        redraw()

canvas.bind("<Button-1>",click)

def toggle_special(bit):
    i=selected[0]
    if maze[i]&bit:
        maze[i]&=~bit
    else:
        maze[i]=(maze[i]&~SPECIAL)|bit
    redraw()

def toggle_bombadeetle():
    i=selected[0]
    if entities[i]&DIR_ALL:
        entities[i]=0
    else:
        entities[i]=last_direction[0]
        enemies[i]=0
    redraw()

def toggle_enemy():
    i=selected[0]
    if enemies[i]&DIR_ALL:
        enemies[i]=0
    else:
        enemies[i]=last_direction[0]
        entities[i]=0
    redraw()

def toggle_goal():
    toggle_special(GOAL)

def toggle_hole():
    toggle_special(HOLE)

def set_selected_direction(direction):
    last_direction[0]=direction
    i=selected[0]
    changed=False
    if entities[i]&DIR_ALL:
        entities[i]=direction
        changed=True
    if enemies[i]&DIR_ALL:
        enemies[i]=direction
        changed=True
    if changed:
        redraw()

def on_key(event):
    key=event.keysym.lower()
    if key=="w":
        set_selected_direction(DIR_N)
    elif key=="a":
        set_selected_direction(DIR_W)
    elif key=="s":
        set_selected_direction(DIR_S)
    elif key=="d":
        set_selected_direction(DIR_E)

root.bind("<Key>",on_key)

def new_map():
    for i in range(len(maze)):
        maze[i]=0
        entities[i]=0
        enemies[i]=0
    selected[0]=0
    current_path[0]=None
    set_name("")
    set_arrow_counts(0,0,0,0)
    update_title()
    redraw()

def get_name():
    return name_var.get()[:NAME_MAX]

def set_name(name=""):
    name_var.set(str(name)[:NAME_MAX])

def get_arrow_counts():
    counts={}
    for key,entry in arrow_entries.items():
        text=entry.get().strip()
        if text=="":
            counts[key]=0
            continue
        try:
            counts[key]=int(text)
        except ValueError:
            raise ValueError("Arrow count for %s must be an integer"%key)
        if counts[key]<0:
            raise ValueError("Arrow count for %s cannot be negative"%key)
    return counts

def set_arrow_counts(left=0,up=0,down=0,right=0):
    values={"left":left,"up":up,"down":down,"right":right}
    for key,entry in arrow_entries.items():
        entry.delete(0,tk.END)
        entry.insert(0,str(values[key]))

def load_map_from_path(path):
    with open(path,"r",encoding="utf-8") as f:
        data=json.load(f)

    if isinstance(data,list):
        cells=data
        width,height=GRID_W,GRID_H
        entity_cells=[]
        enemy_cells=[]
        arrows={}
        name=""
    else:
        width=data.get("width",GRID_W)
        height=data.get("height",GRID_H)
        cells=data.get("maze",[])
        entity_cells=data.get("entities",[])
        enemy_cells=data.get("enemies",[])
        arrows=data.get("arrows",{})
        name=data.get("name","")

    if width!=GRID_W or height!=GRID_H:
        raise ValueError("Map must be %dx%d"%(GRID_W,GRID_H))
    if len(cells)!=GRID_W*GRID_H:
        raise ValueError("Map must have %d cells"%(GRID_W*GRID_H))
    if entity_cells and len(entity_cells)!=GRID_W*GRID_H:
        raise ValueError("Entities must have %d cells"%(GRID_W*GRID_H))
    if enemy_cells and len(enemy_cells)!=GRID_W*GRID_H:
        raise ValueError("Enemies must have %d cells"%(GRID_W*GRID_H))

    if entity_cells or enemy_cells:
        for i,v in enumerate(cells):
            maze[i]=sanitize_maze_cell(v)
            entities[i]=sanitize_facing(entity_cells[i]) if entity_cells else 0
            enemies[i]=sanitize_facing(enemy_cells[i]) if enemy_cells else 0
    else:
        migrate_legacy_map(cells)

    sanitize_loaded_map()

    selected[0]=0
    set_name(name)
    set_arrow_counts(
        left=int(arrows.get("left",0)),
        up=int(arrows.get("up",0)),
        down=int(arrows.get("down",0)),
        right=int(arrows.get("right",0)),
    )
    current_path[0]=path
    update_title()
    redraw()

def open_map():
    path=filedialog.askopenfilename(
        title="Open Map",
        initialdir=APP_DIR,
        filetypes=[("JSON maps","*.json"),("All files","*.*")],
    )
    if not path:
        return
    try:
        load_map_from_path(path)
    except Exception as e:
        messagebox.showerror("Open failed",str(e))

def write_map(path):
    data={
        "name": get_name(),
        "width": GRID_W,
        "height": GRID_H,
        "maze": list(maze),
        "entities": list(entities),
        "enemies": list(enemies),
        "arrows": get_arrow_counts(),
    }
    with open(path,"w",encoding="utf-8") as f:
        json.dump(data,f,indent=2)
        f.write("\n")
    current_path[0]=path
    update_title()

def save_map():
    if current_path[0]:
        try:
            write_map(current_path[0])
        except Exception as e:
            messagebox.showerror("Save failed",str(e))
    else:
        save_map_as()

def save_map_as():
    path=filedialog.asksaveasfilename(
        title="Save Map As",
        initialdir=APP_DIR,
        defaultextension=".json",
        filetypes=[("JSON maps","*.json"),("All files","*.*")],
    )
    if not path:
        return
    try:
        write_map(path)
    except Exception as e:
        messagebox.showerror("Save failed",str(e))

def export_grid(out,name,grid):
    out.append("uint8_t %s[%d] ="%(name,GRID_W*GRID_H))
    out.append("{")
    for y in range(GRID_H):
        row=[]
        for x in range(GRID_W):
            row.append(str(grid[idx(x,y)]))
        out.append("    "+", ".join(row)+",")
    out.append("};")

def export():
    try:
        arrows=get_arrow_counts()
        name=get_name()
    except Exception as e:
        messagebox.showerror("Export failed",str(e))
        return

    win=tk.Toplevel(root)
    win.title("Export C Array")
    txt=tk.Text(win,width=110,height=32)
    txt.pack(fill="both",expand=True)

    escaped=name.replace("\\","\\\\").replace('"','\\"')
    out=['char level_name[%d] = "%s";'%(NAME_MAX+1,escaped),""]
    export_grid(out,"maze",maze)
    out.append("")
    export_grid(out,"entities",entities)
    out.append("")
    export_grid(out,"enemies",enemies)
    out.append("")
    out.append("int8_t arrows_left = %d;"%arrows["left"])
    out.append("int8_t arrows_up = %d;"%arrows["up"])
    out.append("int8_t arrows_down = %d;"%arrows["down"])
    out.append("int8_t arrows_right = %d;"%arrows["right"])
    txt.insert("1.0","\n".join(out))

def export_binary():
    try:
        arrows=get_arrow_counts()
        for key,value in arrows.items():
            if value>127:
                raise ValueError("Arrow count for %s exceeds int8_t max (127)"%key)
        name_bytes=get_name().encode("ascii",errors="replace")[:NAME_MAX]
        name_bytes=name_bytes.ljust(NAME_MAX,b" ")
    except Exception as e:
        messagebox.showerror("Export failed",str(e))
        return

    path=filedialog.asksaveasfilename(
        title="Export Binary File",
        initialdir=APP_DIR,
        defaultextension=".bin",
        filetypes=[("Bombadeetle binary","*.bin"),("All files","*.*")],
    )
    if not path:
        return

    try:
        # name(16) + maze(108) + entities(108) + enemies(108) + arrows(4)
        payload=name_bytes+struct.pack(
            "<%dB%dB%dB4b"%(GRID_W*GRID_H,GRID_W*GRID_H,GRID_W*GRID_H),
            *[v&0xFF for v in maze],
            *[v&0xFF for v in entities],
            *[v&0xFF for v in enemies],
            arrows["left"],
            arrows["up"],
            arrows["down"],
            arrows["right"],
        )
        with open(path,"wb") as f:
            f.write(payload)
    except Exception as e:
        messagebox.showerror("Export failed",str(e))

file_menu.add_command(label="New",command=new_map,accelerator="Ctrl+N")
file_menu.add_command(label="Open...",command=open_map,accelerator="Ctrl+O")
file_menu.add_command(label="Save",command=save_map,accelerator="Ctrl+S")
file_menu.add_command(label="Save As...",command=save_map_as,accelerator="Ctrl+Shift+S")
file_menu.add_separator()
file_menu.add_command(label="Export C Array",command=export)
file_menu.add_command(label="Export Binary File",command=export_binary)
file_menu.add_separator()
file_menu.add_command(label="Exit",command=root.destroy)

root.bind("<Control-n>",lambda e: new_map())
root.bind("<Control-o>",lambda e: open_map())
root.bind("<Control-s>",lambda e: save_map())
root.bind("<Control-S>",lambda e: save_map_as())

name_frame=tk.Frame(root)
name_frame.pack(pady=(8,0))
tk.Label(name_frame,text="Name").pack(side="left",padx=(0,6))
name_var=tk.StringVar()

def _limit_name(*_):
    value=name_var.get()
    if len(value)>NAME_MAX:
        name_var.set(value[:NAME_MAX])

name_var.trace_add("write",_limit_name)
name_entry=tk.Entry(name_frame,textvariable=name_var,width=NAME_MAX+2)
name_entry.pack(side="left")

btns=tk.Frame(root)
btns.pack(pady=5)
tk.Button(btns,text="Bombadeetle",command=toggle_bombadeetle).pack(side="left",padx=4)
tk.Button(btns,text="Goal",command=toggle_goal).pack(side="left",padx=4)
tk.Button(btns,text="Hole",command=toggle_hole).pack(side="left",padx=4)
tk.Button(btns,text="Enemy",command=toggle_enemy).pack(side="left",padx=4)

hint=tk.Label(root,text="Select a Bombadeetle/Enemy tile, then use W/A/S/D to set facing.")
hint.pack(pady=(0,4))

arrows_frame=tk.LabelFrame(root,text="Arrow counts",padx=8,pady=6)
arrows_frame.pack(pady=5)

arrow_entries={}
for col,key,label in ((0,"left","Left"),(1,"up","Up"),(2,"down","Down"),(3,"right","Right")):
    tk.Label(arrows_frame,text=label).grid(row=0,column=col,padx=8)
    entry=tk.Entry(arrows_frame,width=6,justify="center")
    entry.insert(0,"0")
    entry.grid(row=1,column=col,padx=8,pady=2)
    arrow_entries[key]=entry

redraw()
root.mainloop()
