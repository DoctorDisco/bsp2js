/*
 * bsp2js
 *
 * Copyright (C) 2012-2013 Florian Zwoch <fzwoch@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct entry_t {
    uint32_t offset;
    uint32_t size;
};

struct header_t {
    uint32_t version;
    struct entry_t entities;
    struct entry_t planes;
    struct entry_t miptex;
    struct entry_t vertices;
    struct entry_t visilist;
    struct entry_t nodes;
    struct entry_t texinfo;
    struct entry_t faces;
    struct entry_t lightmaps;
    struct entry_t clipnodes;
    struct entry_t leaves;
    struct entry_t faces_list;
    struct entry_t edges;
    struct entry_t edges_list;
    struct entry_t models;
};

struct vec3_t {
    float x;
    float y;
    float z;
};

struct boundbox_t {
    struct vec3_t min;
    struct vec3_t max;
};

struct model_t {
    struct boundbox_t bound;
    struct vec3_t origin;
    uint32_t node_id0;
    uint32_t node_id1;
    uint32_t node_id2;
    uint32_t node_id3;
    uint32_t numleaves;
    uint32_t face_id;
    uint32_t face_num;
};

struct mipheader_t {
    uint32_t numtex;
    uint32_t offsets[];
};

struct miptex_t {
    char name[16];
    uint32_t width;
    uint32_t height;
    uint32_t offset1;
    uint32_t offset2;
    uint32_t offset4;
    uint32_t offset8;
};

struct surface_t {
    struct vec3_t vectorS;
    float distS;
    struct vec3_t vectorT;
    float distT;
    uint32_t texture_id;
    uint32_t animated;
};

struct edge_t {
    uint16_t vertex0;
    uint16_t vertex1;
};

struct face_t {
    uint16_t plane_id;
    uint16_t side;
    int32_t ledge_id;
    uint16_t ledge_num;
    uint16_t texinfo_id;
    uint8_t typelight;
    uint8_t baselight;
    uint8_t light[2];
    int32_t lightmap;
};

int main(int argc, char *argv[])
{
    FILE *fp;
    size_t size;
    uint8_t *ptr;
    uint32_t i;
    uint32_t count = 0;

    struct header_t *header;
    struct model_t *models;
    struct mipheader_t *mipheader;
    struct vec3_t *vertex;
    struct surface_t *surfaces;
    int16_t *faces_list;
    struct face_t *faces;
    int32_t *edges_list;
    struct edge_t *edges;

    if (argc != 2)
    {
        printf("usage: %s <map.bsp>\n", argv[0]);
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp)
    {
        printf("could not open file: %s\n", argv[1]);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    ptr = (uint8_t *) malloc(size);
    if (!ptr)
    {
        printf("could not allocate memory\n");
        return 1;
    }

    fread(ptr, size, 1, fp);
    fclose(fp);

    header = (struct header_t *) ptr;
    models = (struct model_t *) (ptr + header->models.offset);
    mipheader = (struct mipheader_t *) (ptr + header->miptex.offset);
    vertex = (struct vec3_t *) (ptr + header->vertices.offset);
    surfaces = (struct surface_t *) (ptr + header->texinfo.offset);
    faces_list = (int16_t *) (ptr + header->faces_list.offset);
    faces = (struct face_t *) (ptr + header->faces.offset);
    edges_list = (int32_t *) (ptr + header->edges_list.offset);
    edges = (struct edge_t *) (ptr + header->edges.offset);

    printf("{\n");
    printf("    \"metadata\": { \"formatVersion\" : 3 },\n");
    printf("    \"materials\": [\n");

    for (i = 0; i < mipheader->numtex; i++)
    {
        struct miptex_t *miptex = (struct miptex_t *) (ptr + header->miptex.offset + mipheader->offsets[i]);

        if (mipheader->offsets[i] == -1 || strcmp(miptex->name, "trigger") == 0 || strcmp(miptex->name, "clip") == 0 || strcmp(miptex->name, "black") == 0)
        {
            printf("        {\n");
            printf("            \"DbgName\": \"dummy\"\n");
            printf("        }%s\n", i == mipheader->numtex - 1 ? "" : ",");

            continue;
        }

        if (miptex->name[0] == '*')
        {
            miptex->name[0] = '+';
        }

        printf("        {\n");
        printf("            \"mapDiffuse\": \"%s.jpg\",\n", miptex->name);
        printf("            \"mapDiffuseWrap\": [\"repeat\", \"repeat\"]\n");
        printf("        }%s\n", i == mipheader->numtex - 1 ? "" : ",");
    }

    printf("    ],\n");
    printf("    \"vertices\": [");

    for (i = 0; i < header->vertices.size / sizeof(struct vec3_t); i++)
    {
        printf("%g,%g,%g%s", vertex[i].x, vertex[i].y, vertex[i].z, i == header->vertices.size / sizeof(struct vec3_t) - 1 ? "" : ",");
    }

    printf("],\n");
    printf("    \"uvs\": [ [");

    for (i = 0; i < models[0].face_num; i++)
    {
        uint16_t j;
        struct face_t *face = &faces[models[0].face_id + i];
        struct edge_t *edge = edges + abs(edges_list[face->ledge_id]);
        struct miptex_t *miptex = (struct miptex_t *) (ptr + header->miptex.offset + mipheader->offsets[surfaces[face->texinfo_id].texture_id]);

        if (strcmp(miptex->name, "trigger") == 0 || strcmp(miptex->name, "clip") == 0 || strcmp(miptex->name, "black") == 0)
        {
            continue;
        }

        for (j = 1; j < face->ledge_num - 1; j++)
        {
            float u[3];
            float v[3];
            struct edge_t *edge1 = edges + abs(edges_list[face->ledge_id + j]);
            struct edge_t *edge2 = edges + abs(edges_list[face->ledge_id + j + 1]);

            u[0] = (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].x : vertex[edge->vertex0].x) * surfaces[face->texinfo_id].vectorS.x + (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].y : vertex[edge->vertex0].y) * surfaces[face->texinfo_id].vectorS.y + (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].z : vertex[edge->vertex0].z) * surfaces[face->texinfo_id].vectorS.z + surfaces[face->texinfo_id].distS;
            v[0] = (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].x : vertex[edge->vertex0].x) * surfaces[face->texinfo_id].vectorT.x + (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].y : vertex[edge->vertex0].y) * surfaces[face->texinfo_id].vectorT.y + (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].z : vertex[edge->vertex0].z) * surfaces[face->texinfo_id].vectorT.z + surfaces[face->texinfo_id].distT;

            u[1] = (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].x : vertex[edge1->vertex0].x) * surfaces[face->texinfo_id].vectorS.x + (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].y : vertex[edge1->vertex0].y) * surfaces[face->texinfo_id].vectorS.y + (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].z : vertex[edge1->vertex0].z) * surfaces[face->texinfo_id].vectorS.z + surfaces[face->texinfo_id].distS;
            v[1] = (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].x : vertex[edge1->vertex0].x) * surfaces[face->texinfo_id].vectorT.x + (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].y : vertex[edge1->vertex0].y) * surfaces[face->texinfo_id].vectorT.y + (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].z : vertex[edge1->vertex0].z) * surfaces[face->texinfo_id].vectorT.z + surfaces[face->texinfo_id].distT;

            u[2] = (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].x : vertex[edge2->vertex0].x) * surfaces[face->texinfo_id].vectorS.x + (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].y : vertex[edge2->vertex0].y) * surfaces[face->texinfo_id].vectorS.y + (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].z : vertex[edge2->vertex0].z) * surfaces[face->texinfo_id].vectorS.z + surfaces[face->texinfo_id].distS;
            v[2] = (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].x : vertex[edge2->vertex0].x) * surfaces[face->texinfo_id].vectorT.x + (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].y : vertex[edge2->vertex0].y) * surfaces[face->texinfo_id].vectorT.y + (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].z : vertex[edge2->vertex0].z) * surfaces[face->texinfo_id].vectorT.z + surfaces[face->texinfo_id].distT;

            printf("%g,%g,%g,%g,%g,%g%s", u[0] / miptex->width, 1 - v[0] / miptex->height, u[1] / miptex->width, 1 - v[1] / miptex->height, u[2] / miptex->width, 1 - v[2] / miptex->height, i == models[0].face_num - 1 && j == face->ledge_num - 2 ? "" : ",");
        }
    }

    printf("] ],\n");
    printf("    \"faces\": [");

    for (i = 0; i < models[0].face_num; i++)
    {
        uint16_t j;
        struct face_t *face = &faces[models[0].face_id + i];
        struct edge_t *edge = edges + abs(edges_list[face->ledge_id]);
        struct miptex_t *miptex = (struct miptex_t *) (ptr + header->miptex.offset + mipheader->offsets[surfaces[face->texinfo_id].texture_id]);

        if (strcmp(miptex->name, "trigger") == 0)
        {
            continue;
        }

        for (j = 1; j < face->ledge_num - 1; j++)
        {
            struct edge_t *edge1 = edges + abs(edges_list[face->ledge_id + j]);
            struct edge_t *edge2 = edges + abs(edges_list[face->ledge_id + j + 1]);

            printf("10,%u,%u,%u,%u,%d,%d,%d%s", edges_list[face->ledge_id] < 0 ? edge->vertex1 : edge->vertex0, edges_list[face->ledge_id + j + 1] < 0 ? edge2->vertex1 : edge2->vertex0, edges_list[face->ledge_id + j] < 0 ? edge1->vertex1 : edge1->vertex0, surfaces[face->texinfo_id].texture_id, count, count + 2, count + 1, i == models[0].face_num - 1 && j == face->ledge_num - 2 ? "" : ",");

            count += 3;
        }
    }

    printf("]\n");
    printf("}\n");

    free(ptr);

    return 0;
}
