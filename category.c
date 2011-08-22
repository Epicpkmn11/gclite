/*
 *  this file is part of Game Categories Lite
 *
 *  Copyright (C) 2011  Bubbletune
 *  Copyright (C) 2011  Codestation
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pspkernel.h>
#include <psprtc.h>
#include "categories_lite.h"
#include "psppaf.h"
#include "config.h"
#include "logger.h"

const char *eboot_types[] = { "EBOOT.PBP", "PARAM.PBP", "PBOOT.PBP" };

Category *first_category[2] = { NULL, NULL };

Category *GetNextCategory(Category *prev, int location) {
    u64 time = 0, last;
    Category *newest = NULL;

    if (prev) {
        last = prev->mtime;
    } else {
        last = (u64) -1;
    }
    Category *p = (Category *) first_category[location];

    while (p) {
        if (p->mtime < last) {
            if (p->mtime > time) {
                time = p->mtime;
                newest = p;
            }
        }

        p = p->next;
    }

    return newest;
}

void ClearCategories(int location) {
    Category *next;
    Category *p = (void *) first_category[location];

    while (p) {
        next = p->next;
        sce_paf_private_free(p);
        p = next;
    }

    first_category[location] = NULL;
}

int CountCategories(int location) {
    int i = 0;
    Category *p = (void *) first_category[location];

    while (p) {
        i++;
        p = p->next;
    }

    return i;
}

void AddCategory(char *category, u64 mtime, int location) {
    Category *p, *category_entry;

    while (1) {
        p = NULL;
        while ((p = GetNextCategory(p, location))) {
            if (sce_paf_private_strcmp(category, &p->name) == 0) {
                return;
            }
            if (p->mtime == mtime) {
                mtime++;
                break;
            }
        }
        if (!p) {
            break;
        }
    }
    category_entry = (Category *) sce_paf_private_malloc(sizeof(Category) + sce_paf_private_strlen(category) + 1);
    if (category_entry) {
        category_entry->next = NULL;
        category_entry->mtime = mtime;
        category_entry->location = location;
        sce_paf_private_strcpy(&category_entry->name, category);

        if (!first_category[location]) {
            first_category[location] = category_entry;
        } else {
            p = (Category *) first_category[location];
            while (p->next) {
                p = p->next;
            }
            p->next = category_entry;
        }
    }
}

void DelCategory(char *category, int location) {
    Category *prev = NULL;
    Category *p = (Category *) first_category[location];

    while (p) {
        if (sce_paf_private_strcmp(&p->name, category) == 0) {
            if (prev) {
                prev->next = p->next;
            } else {
                first_category[location] = p->next;
            }
            break;
        }
        prev = p;
        p = p->next;
    }
}

Category *FindCategory(const char *category, int location) {
    Category *p = (Category *) first_category[location];
    while(p) {
        if (sce_paf_private_strcmp(&p->name, category) == 0) {
            break;
        }
        p = p->next;
    }
    return p;
}

int is_category(const char *base, const char *path) {
    SceIoStat stat;
    char buffer[256];
    for(u32 i = 0; i < ITEMSOF(eboot_types); i++) {
        sce_paf_private_memset(&stat, 0 , sizeof(SceIoStat));
        sce_paf_private_snprintf(buffer, 256, "%s/%s/%s", base, path, eboot_types[i]);
        if(sceIoGetstat(buffer, &stat) >= 0) {
            return 0;
        }
    }
    return 1;
}

void IndexCategories(const char *path, int location) {
    SceIoDirent dir;
    SceUID fd;
    u64 mtime;
    char full_path[16];
    int match;

    sce_paf_private_strcpy(full_path, path);
    SET_DEVICENAME(full_path, location);

    if((fd = sceIoDopen(full_path)) < 0) {
        return;
    }

    kprintf("Indexing categories from %s, loc: %i\n", path, location);
    match = 0;
    sce_paf_private_memset(&dir, 0, sizeof(SceIoDirent));

    while(1) {
        if(sceIoDread(fd, &dir) <= 0) {
            kprintf("End of directory list\n");
            sceIoDclose(fd);
            break;
        }
        kprintf("checking %s, length: %i\n", dir.d_name, sce_paf_private_strlen(dir.d_name));
        if (FIO_S_ISDIR(dir.d_stat.st_mode) && dir.d_name[0] != '.') {
            if(!config.prefix && is_category(full_path, dir.d_name)) {
                match = 1;
            }else if(config.prefix && sce_paf_private_strncmp(dir.d_name, "CAT_", 4) == 0) {
                sce_paf_private_strcpy(dir.d_name, dir.d_name + 4);
                match = 1;
            }
            if(match) {
                match = 0;
                sceRtcGetTick((pspTime *) &dir.d_stat.st_mtime, &mtime);
                kprintf("Adding %s as category\n", dir.d_name);
                AddCategory(dir.d_name, mtime, location);
            }
        }
    }
}
