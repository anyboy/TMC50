#!/usr/bin/env python3

import sys

if len(sys.argv) != 2:
    print("missing input file")
    sys.exit()

f = open(sys.argv[1], 'r')
lines = f.read().split('\n')
f.close()

next_l = 0
prev_data0 = ""

freemem_bottom = 0
freemem_top = 0

__overlay_data_start = 0
__overlay_data_end = 0
__overlay_bss_start = 0
__overlay_bss_end = 0
__overlay_bss_size = 0
__overlay_bss_name = "overlay.bss"

sections_name = []
sections_list = []

sections_key = ['text', 'rodata', '.codec_stack', 'ram_mpool', '.bthost_bss',
                '.btcon_bss', '.media_bss', '.diskio_cache_bss',
                '.sys_bss', 'noinit', '.bt_bss', 'bss',
                'initlevel', 'datas', 'rom_dependent_ram',
                __overlay_bss_name]

def add_sections_list(item):
    global freemem_top
    if len(sections_key) == 0:
        sections_list.append(item)
    else:
        for key in sections_key:
            if key == item[0]:
                sections_list.append(item)
                if item[0] == '.btcon_bss':
                    freemem_top = item[1]
                    #print(freemem_top)

def is_nonzero_number(s):
    try:
        num = int(s, 16)
        if num != 0:
            return True
        else:
            return False
    except ValueError:
        #print("%s ValueError" % s)
        return False

def tabulize(s, width = 3):
    tab_size = 8
    t = int(len(s) / tab_size)
    return s + ''.join('\t' * (width -t))

def hex_tab(s, width = 1):
    return tabulize(hex(int(s,16)), width)

for l in lines:
    data = " ".join(l.split()).split(" ")
    #print(data)
    
    if next_l == 1:
        next_l = 0
        if len(data) == 2 and is_nonzero_number(data[0]) and is_nonzero_number(data[1]):
            sections_name.append(prev_data0)
            #print(tabulize(prev_data0) + hex_tab(data[0]) + '\t' + data[1])
            add_sections_list((prev_data0, int(data[0],16), int(data[1],16)))
    
    if len(data) == 1:
        if data[0] in sections_name:
            continue
        next_l = 1
        prev_data0 = data[0]
        continue
    elif len(data) >= 3 and is_nonzero_number(data[1]) and is_nonzero_number(data[2]):
        if data[0] in sections_name:
            continue
        sections_name.append(data[0])
        #print(tabulize(data[0]) + hex_tab(data[1]) + '\t' + data[2])
        add_sections_list((data[0], int(data[1],16), int(data[2],16)))

    if len(data) > 2:
        if data[1] == '__overlay_data_start':
            __overlay_data_start = int(data[0], 16)
            #print("overlay data start 0x%x" % __overlay_data_start)
        elif data[1] == '__data_ram_end':
            __overlay_data_end = int(data[0], 16)
            #print("overlay data end 0x%x" % __overlay_data_end)
        elif data[1] == '_media_al_memory_start':
            __overlay_bss_start = int(data[0], 16)
            #print("overlay bss start 0x%x" % __overlay_bss_start)
        elif data[1] == '__overlay_bss_max_end':
            __overlay_bss_end = int(data[0], 16)
            #print("overlay bss end 0x%x" % __overlay_bss_end)

print("Memory Mapping:")

if __overlay_bss_end > __overlay_bss_start:
    __overlay_bss_size = __overlay_bss_end - __overlay_bss_start
    add_sections_list((__overlay_bss_name, __overlay_bss_start, __overlay_bss_size))

sections_list.sort(key=lambda x: x[1], reverse=False)
for s in sections_list:
    print(tabulize(s[0]) + hex_tab(hex(s[1])) + '\t' + hex(s[2]))

freemem_bottom = __overlay_bss_end

if freemem_top > freemem_bottom:
    print(tabulize('free_memory') + hex_tab(hex(freemem_bottom)) + '\t' + hex(freemem_top - freemem_bottom))
else:
    print("error free memory => bottom:%x top:%x" %(freemem_bottom, freemem_top))


