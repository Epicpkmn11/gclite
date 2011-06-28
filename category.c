/*
 Game Categories Light v 1.3
 Copyright (C) 2011, Bubbletune

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

#include <pspkernel.h>
#include <psprtc.h>
#include "game_categories_light.h"
#include "psppaf.h"

Category *first_category = NULL;

Category *GetNextCategory(Category *prev) {
    u64 time = 0, last;
    Category *newest = NULL;

    if (prev) {
        last = prev->mtime;
    } else {
        last = (u64) -1;
    }
    Category *p = (Category *) first_category;
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

void ClearCategories() {
    Category *next;
    Category *p = (void *) first_category;

    while (p) {
        next = p->next;
        sce_paf_private_free(p);
        p = next;
    }

    first_category = NULL;
}

int CountCategories() {
    int i = 0;
    Category *p = (void *) first_category;

    while (p) {
        i++;
        p = p->next;
    }

    return i;
}

void AddCategory(char *category, u64 mtime) {
    Category *p, *category_entry;

    while (1) {
        p = NULL;
        while ((p = GetNextCategory(p))) {
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
        sce_paf_private_strcpy(&category_entry->name, category);

        if (!first_category) {
            first_category = category_entry;
        } else {
            p = (Category *) first_category;
            while (p->next) {
                p = p->next;
            }
            p->next = category_entry;
        }
    }
    return;
}

void DelCategory(char *category) {
    Category *prev = NULL;
    Category *p = (Category *) first_category;

    while (p) {
        if (sce_paf_private_strcmp(&p->name, category) == 0) {
            if (prev) {
                prev->next = p->next;
            } else {
                first_category = p->next;
            }
            break;
        }
        prev = p;
        p = p->next;
    }
}

void IndexCategories() {
    SceIoDirent dir;
    SceUID fd;
    u64 mtime;

    ClearCategories();
    if((fd = sceIoDopen("ms0:/PSP/GAME")) < 0)
        return;
    sce_paf_private_memset(&dir, 0, sizeof(SceIoDirent));
    while(1) {
        if(sceIoDread(fd, &dir) <= 0) {
            sceIoDclose(fd);
            break;
        }
        if (FIO_S_ISDIR(dir.d_stat.st_mode) && sce_paf_private_strncmp(dir.d_name, "CAT_", 4) == 0) {
            sceRtcGetTick((pspTime *) &dir.d_stat.st_mtime, &mtime);
            sce_paf_private_strcpy(dir.d_name, dir.d_name + 4);
            AddCategory(dir.d_name, mtime);
        }
    }
}