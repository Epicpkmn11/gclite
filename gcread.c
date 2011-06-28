/*
	Game Categories Lite 1.0
	Copyright (C) 2011, codestation
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <pspsdk.h>
#include <pspkernel.h>
#include <psprtc.h>
#include "psppaf.h"
#include <string.h>
#include "game_categories_light.h"
#include "pspdefs.h"
#include "logger.h"

char user_buffer[256];
char category[52];

char mod_path[64];
char orig_path[64];
int multi_cat = 0;
SceUID catdfd = -1;

int is_iso_cat() {
    sce_paf_private_snprintf(user_buffer, 256, "ms0:/ISO/CAT_%s", category);
    SceIoStat st;
    memset(&st, 0, sizeof(SceIoStat));
    if(sceIoGetstat(user_buffer, &st) >= 0 && FIO_S_ISDIR(st.st_mode)) {
        return 1;
    }
    return 0;
}

inline void fix_path(char **path) {
    if(*category && sce_paf_private_strcmp(*path, mod_path) == 0 && is_iso_cat()) {
        *path = orig_path;
    }
}

int is_category_folder(SceIoDirent *dir, char *cat) {
    if(FIO_S_ISDIR(dir->d_stat.st_mode)) {
        if(sce_paf_private_strncmp(dir->d_name, "CAT_", 4) == 0) {
            if(!cat || sce_paf_private_strcmp(dir->d_name + 4, cat) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

SceUID open_iso_cat(SceUID fd) {
    SceIoDirent dir;
    if(fd >= 0) {
        memset(&dir, 0, sizeof(SceIoDirent));
        while(1) {
            int res = sceIoDread(fd, &dir);
            if(res > 0) {
                kprintf("open_iso_cat, check: %s\n", dir.d_name);
                if(is_category_folder(&dir, category)) {
                    // full path
                    sce_paf_private_snprintf(user_buffer, 256, "%s/%s", orig_path, dir.d_name);
                    kprintf("open_iso_cat, opening: %s\n", user_buffer);
                    fd = sceIoDopen(user_buffer);
                    break;
                }
            } else {
                fd = -1;
                break;
            }
        }
    }
    return fd;
}

SceUID sceIoDopenPatched(const char *path) {
    kprintf("sceIoDopenPatched called: %s\n", path);
    if(*category && sce_paf_private_strcmp(path, mod_path) == 0 && is_iso_cat()) {
        multi_cat = 1;
        path = orig_path;
        kprintf("Changed path to: %s\n", path);
    }
    return sceIoDopen(path);
}

int sceIoDreadPatched(SceUID fd, SceIoDirent *dir) {
    int res = -1;
    kprintf("sceIoDreadPatched called\n");
    while(1) {
        if(catdfd >= 0) {
            kprintf("Reading %s\n", category);
            res = sceIoDread(catdfd, dir);
            if(res <= 0) {
                sceIoDclose(catdfd);
                kprintf("Open next category\n");
                if((catdfd = open_iso_cat(fd)) < 0) {
                    multi_cat = 0;
                    break;
                }
                continue;
            }
            kprintf("Read %s\n", dir->d_name);
            break;
        }
        if(multi_cat) {
            kprintf("Found iso category: %s\n", category);
            if((catdfd = open_iso_cat(fd)) < 0) {
                multi_cat = 0;
                break;
            }
            continue;
        }
        res = sceIoDread(fd, dir);
        // filter out category folders in uncategorized view
        if(category[0] == '\0' && res > 0) {
            kprintf("Checking: %s\n", dir->d_name);
            if(dir->d_name[0] == '.' || is_category_folder(dir, NULL) ||
                    sce_paf_private_strcmp(dir->d_name, "VIDEO") == 0) { // skip the VIDEO folder too
                kprintf("Skipping %s\n", dir->d_name);
                continue;
            }
        }
        break;
    }
    return res;
}

int sceIoGetstatPatched(char *file, SceIoStat *stat) {
    fix_path(&file);
    return sceIoGetstat(file, stat);
}

int sceIoChstatPatched(char *file, SceIoStat *stat, int bits) {
    fix_path(&file);
    return sceIoChstat(file, stat, bits);
}

int sceIoRemovePatched(char *file) {
    fix_path(&file);
    return sceIoRemove(file);
}

int sceIoRmdirPatched(char *path) {
    fix_path(&path);
    return sceIoRmdir(path);
}

char *ReturnBasePathPatched(char *base) {
    if(*category && base && sce_paf_private_strcmp(base + 4, "/PSP/GAME") == 0) {
        sce_paf_private_strcpy(orig_path, base);
        sce_paf_private_strcpy(mod_path, base);
        sce_paf_private_strcpy(mod_path + 13, "/CAT_");
        sce_paf_private_strcpy(mod_path + 18, category);
        return mod_path;
    }
    return base;
}
