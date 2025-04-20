#include "engine/hierarchy.h"
#include "raylib.h"
#include "typedef.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

memoryPool *poolPtr;
objectSpace *workspace;
size_t memory_busy = 0;
size_t memory_busyTimes = 0;
size_t memory_clearTimes = 0;

//
// === MEMORY MANAGEMENT ===
//

void *readable_malloc(size_t len) {
   if (len == 0) return NULL;
   void *ptr = malloc(len);
   if (ptr) {
      memory_busy += len;
      memory_busyTimes += 1;
   }
   printf("addr: %p memory added: %zu memory capacity: %zu \n\n", ptr, len, memory_busy);
   return ptr;
}

void *readable_realloc(void *ptr, size_t len, size_t oldlen) {
   if (len == 0) return NULL;
   void *newptr = realloc(ptr, len);
   if (newptr) {
      memory_busy = memory_busy - oldlen + len;
      memory_busyTimes += 1;
   }
   return newptr;
}

void free_readable_malloc(void *ptr, size_t len) {
   if (!ptr) return;
   free(ptr);
   memory_busy = (memory_busy < len) ? 0 : memory_busy - len;
   memory_clearTimes += 1;
   printf("memory cleared: %zu memory capacity: %zu \n", len, memory_busy);
}

char *str_dup(const char *str) {
   size_t len = strlen(str) + 1;
   char *newStr = readable_malloc(len);
   if (newStr) {
      strcpy(newStr, str);
   }
   return newStr;
}

void changePathVar(char **ptr, const char *str) {
   if (!ptr || !str) return;
   if (*ptr) {
      free_readable_malloc(*ptr, strlen(*ptr));
   }
   *ptr = str_dup(str);
}

//
// === POOL ===
//

memoryPool *initPool(size_t size) {
   if (!size) {
      LOG_ERR("where size in initPool()");
      return NULL;
   }
   memoryPool *pool = readable_malloc(sizeof(memoryPool));
   if (!pool) {
      LOG_ERR("pool is null");
      return NULL;
   }

   pool->memory = readable_malloc(size);
   if (!pool->memory) {
      LOG_ERR("pool memory is null");
      free_readable_malloc(pool, sizeof(memoryPool));
      return NULL;
   }

   pool->size = size;
   pool->used = 0;
   pool->free_count = 0;
   pool->capacity = 100;
   pool->indexFree = readable_malloc(sizeof(poolIndexFree) * pool->capacity);

   if (createIndexFree(pool, 0, size) <= 0) {
      LOG_ERR("pool indexFree is null");
      free_readable_malloc(pool->memory, size);
      free_readable_malloc(pool, sizeof(memoryPool));
      return NULL;
   }

   return pool;
}

u_int_8 createIndexFree(memoryPool *pool, size_t start, size_t end) {
   if (!pool || !start || !end) {
      LOG_ERR("where my pool in createIndexFree");
      return -1;
   }
   if (pool->capacity <= pool->free_count) {
      size_t new_capacity = pool->capacity * 2;
      poolIndexFree *poolIndexFreePtr = pool->indexFree = readable_realloc(pool->indexFree, sizeof(poolIndexFree) * new_capacity, sizeof(poolIndexFree) * pool->capacity);
      if (!poolIndexFreePtr) {
         LOG_ERR("indexFree in createIndexFree is null");
         return -1;
      }

      pool->indexFree = poolIndexFreePtr;
      pool->capacity = new_capacity;
   }

   pool->indexFree[pool->free_count].start = 0;
   pool->indexFree[pool->free_count].end = end - 1;
   pool->free_count = pool->free_count + 1;
   return 1;
}

void deleteIndexFree(memoryPool *pool, u_int_32 index) {
   if (!pool || !index) {
      LOG_ERR("where my pool in createIndexFree");
      return;
   }

   if (!pool->indexFree[index].start && !pool->indexFree[index].end) {
      LOG_ERR("no indexFree in deleteIndexFree");
      return;
   }
   
   for (u_int_32 i = index; i < pool->free_count - 1; i++) {
      pool->indexFree[i] = pool->indexFree[i + 1]; 
   }

   pool->indexFree[pool->free_count - 1].start = 0;
   pool->indexFree[pool->free_count - 1].end = 0;

}

void *addPoolChild(memoryPool *gottenPool, void *data, size_t size) {
   if (!gottenPool || !data || !size) {
      LOG_ERR("where my data in addPoolChild");
      return NULL;
   }
return NULL;
}

//
// === TYPE CHECKING ===
//

bool checkIsTypeWorkspace(void *parent) {
   if (!parent) return false;
   baseNode *node = getBaseNode(parent); 
   return (node && strcmp(node->name, "workspace") == 0);
}

kindStructType getIsKindType(void *parent) {
   if (!parent) return PARENT_TYPE_NULL;
   baseNode *node = getBaseNode(parent);
   return node->kindtype;
}

baseNode *getBaseNode(void *ptr) {
   return (baseNode*)ptr;
}

void setDefaultBaseNode(baseNode *node, void *parent) {
   if (!node) return;
   node->name = "";
   node->parent = parent;
   node->children = NULL;
   node->childCount = 0;
   node->kindtype = PARENT_TYPE_NULL;
   node->variables = malloc(8);
   node->variablesSize = 8;
}

void clearBaseNode(baseNode *node) {
   if (!node) return;
   if (node->variables) {
      free_readable_malloc(node->variables, node->variablesSize);
      node->variables = NULL;
      node->variablesSize = 0;
   }
}

//
// === CHILD MANAGEMENT ===
//

void changeVoidChild(void ***childrenPtr, int *childCount, void *child) {
   size_t oldSize = sizeof(void *) * (*childCount);
   size_t newSize = sizeof(void *) * (*childCount + 1);
   void **newChildren = readable_realloc(*childrenPtr, newSize, oldSize);
   if (!newChildren) return;

   *childrenPtr = newChildren;
   (*childrenPtr)[*childCount] = child;
   *childCount += 1;
}

void childAdd__(void *parent, void *child) {
   if (!parent || !child) return;
   baseNode *node = getBaseNode(parent);
   if (!node) return;
   changeVoidChild(&node->children, &node->childCount, child);
}

void removeChildrens(void ***children, int *childCount) {
   if (!children || !*children || *childCount <= 0) return;

   for (int index = 0; index < *childCount; index++) {
      void *child = (*children)[index];
      if (!child) {
         LOG_WARN("removeChildrens: null child");
         continue;
      }
      LOG_WARN("hi");
      baseNode *node = getBaseNode(child);
      if (node->kindtype == PARENT_TYPE_OBJECT) {
         remove_object((objectSpace *)child);
      } else if (node->kindtype == PARENT_TYPE_PART) {
         remove_shape((shapeProperties *)child);
      }
   }
   free_readable_malloc(*children, (*childCount) * sizeof(void*));
   *children = NULL;
   *childCount = 0;
}

//
// === INSTANCE FUNCTIONS ===
//

shapeProperties *instance_shape() {
   return readable_malloc(sizeof(shapeProperties));
}

partCube *instance_cube(void *parent, shapeProperties *shape) {
   partCube *part = readable_malloc(sizeof(partCube));
   if (!part) return NULL;

   shape->part = part;
   shape->partType = OBJECT_TYPE_CUBE;

   LOG_INFO("partCube: created");
   return part;
}

partSphere *instance_sphere(void *parent, shapeProperties *shape, float radius) {
   partSphere *part = readable_malloc(sizeof(partSphere));
   if (!part) return NULL;

   shape->part = part;
   shape->partType = OBJECT_TYPE_SPHERE;
   part->radius = (radius == 0.0f || isnan(radius)) ? 1.0f : radius;

   LOG_INFO("partSphere: created");
   return part;
}

partCylinder *instance_cylinder(void *parent, shapeProperties *shape, float radiusTop, float radiusBottom, float height, int slices) {
   partCylinder *part = readable_malloc(sizeof(partCylinder));
   if (!part) return NULL;

   shape->part = part;
   shape->partType = OBJECT_TYPE_CYLINDER;

   part->radiusTop = (radiusTop == 0.0f || isnan(radiusTop)) ? 1.0f : radiusTop;
   part->radiusBottom = (radiusBottom == 0.0f || isnan(radiusBottom)) ? 1.0f : radiusBottom;
   part->height = (height == 0.0f || isnan(height)) ? 1.0f : height;
   part->slices = (slices == 0 || (slices <= 0)) ? 1 : slices;

   LOG_INFO("partCylinder: created");
   return part;
}

partMesh *instance_meshPart(void *parent, shapeProperties *shape, Mesh *mesh, Texture2D *texture, char *meshPath) {
   partMesh *part = readable_malloc(sizeof(partMesh));
   if (!part) return NULL;

   shape->part = part;
   shape->partType = OBJECT_TYPE_MESHPART;

   part->mesh = mesh;
   part->texture = texture;
   part->meshPath = NULL;
   changePathVar(&part->meshPath, (meshPath ? meshPath : NULL));

   LOG_INFO("partMesh: created");
   return part;
}

shapeProperties *instancePart(void *parent, objectType type, void *params) {
   void *actualParent = parent ? parent : workspace;
   shapeProperties *newShape = instance_shape();
   if (!newShape) {
      LOG_ERR("newShape: cant create in instancePart");
      return NULL;
   }

   setDefaultShape(newShape, parent);
   printf("NEW PARENT FOR OBJECT: %p\n", parent);
   bool success = false;
   switch (type) {
      case OBJECT_TYPE_CUBE:
         success = instance_cube(actualParent, newShape);
         break;
      case OBJECT_TYPE_SPHERE:
         success = instance_sphere(actualParent, newShape, ((partSphere *)params)->radius);
         break;
      case OBJECT_TYPE_CYLINDER: 
         {
            partCylinder *p = (partCylinder *)params;
            success = instance_cylinder(actualParent, newShape, p->radiusTop, p->radiusBottom, p->height, p->slices);
            break;
         }
      case OBJECT_TYPE_MESHPART: 
         {
            partMesh *p = (partMesh *)params;
            success = instance_meshPart(actualParent, newShape, p->mesh, p->texture, p->meshPath);
            break;
         }
      default:
         remove_shape(newShape);
         return NULL;
   }
   if (!success) {
      LOG_ERR("instancePart: null yes");
      remove_shape(newShape);
      return NULL;
   }
   childAdd__(parent, newShape);
   return newShape;
}

//
// === REMOVE FUNCTIONS ===
//

void remove_cube(partCube **part) {
   if (part && *part) {
      free_readable_malloc(*part, sizeof(partCube));
      *part = NULL;
   }
}

void remove_sphere(partSphere **part) {
   if (part && *part) {
      free_readable_malloc(*part, sizeof(partSphere));
      *part = NULL;
   }
}

void remove_cylinder(partCylinder **part) {
   if (part && *part) {
      free_readable_malloc(*part, sizeof(partCylinder));
      *part = NULL;
   }
}

void remove_meshPart(partMesh **part) {
   if (part && *part) {
      if ((*part)->meshPath) {
         free_readable_malloc((*part)->meshPath, strlen((*part)->meshPath) + 1);
      }
      free_readable_malloc(*part, sizeof(partMesh));
      *part = NULL;
   }
}

void removePart(void *part, objectType type) {
   if (!part) return;

   switch (type) {
      case OBJECT_TYPE_CUBE:
         remove_cube((partCube **)&part);
         break;
      case OBJECT_TYPE_SPHERE:
         remove_sphere((partSphere **)&part);
         break;
      case OBJECT_TYPE_CYLINDER:
         remove_cylinder((partCylinder **)&part);
         break;
      case OBJECT_TYPE_MESHPART:
         remove_meshPart((partMesh **)&part);
         break;
      default:
         LOG_WARN("removePart: DEFAULT CASE");
         break;
   }
}

void remove_shape(shapeProperties *shape) {
   if (!shape) return;
   baseNode *node = getBaseNode(shape);

   if (shape->part) {
      removePart(shape->part, shape->partType);
      shape->part = NULL;
   }

   removeChildrens(&node->children, &node->childCount);
   clearBaseNode(node);

   free_readable_malloc(shape, sizeof(*shape));

}

void childDestroy(void *ptr) {
   if (!ptr) return;
   baseNode *node = getBaseNode(ptr);
   if (!node) {
      LOG_WARN("childDestroy: NODE NULL!");
      return;
   }
   switch (node->kindtype) {
      case PARENT_TYPE_OBJECT:
         LOG_WARN("removing PARENT_TYPE_OBJECT");
         remove_object((objectSpace*)ptr); 
         break;
      case PARENT_TYPE_PART:
         LOG_INFO("removing PARENT_TYPE_PART");
         remove_shape((shapeProperties*)ptr);
         break;
      default: return;
   }
}

//
// === OBJECT ===
//

objectSpace *instance_object(void *parent) {
   if (!parent) return NULL;

   LOG_INFO("Adding object");
   objectSpace *object = readable_malloc(sizeof(objectSpace));

   baseNode *node = getBaseNode(object);
   setDefaultBaseNode(node, parent);
   node->kindtype = PARENT_TYPE_OBJECT;

   childAdd__(parent, object);
   return object;
}

void remove_object(objectSpace *object) {
   if (!object) return;
   baseNode *node = getBaseNode(object);
   removeChildrens(&node->children, &node->childCount);
   clearBaseNode(node);
   free_readable_malloc(object, sizeof(*object));
}

//
// === SHAPE DEFAULTS ===
//

int setDefaultShape(shapeProperties *shape, void *parent) {
   if (!shape) return -1;

   baseNode *node = getBaseNode(shape);
   setDefaultBaseNode(node, parent);
   node->kindtype = PARENT_TYPE_PART;

   shape->flags = 0;
   shape->transparency = 255;
   shape->size = (Vector3){2.0, 2.0, 2.0};
   shape->orientation = (Vector3){0.0, 0.0, 0.0};
   shape->material = MATERIAL_TYPE_PLASTIC;
   shape->brickColor = (Color){127, 127, 127, shape->transparency};
   shape->collisionGroup = COLLISION_GROUP_ONE;

   return 0;
}

//
// === WORKSPACE ===
//

void init_workspace(memoryPool *gottenPool) {
   if (!gottenPool) {
      LOG_ERR("where pool in init_workspace");
      return;
   }
   remove_workspace();

   poolPtr = gottenPool;

   workspace = readable_malloc(sizeof(objectSpace));
   baseNode node = workspace->mynode;
   setDefaultBaseNode(&node, NULL);
   node.kindtype = PARENT_TYPE_OBJECT;
   node.name = "workspace";

   printf("WORKSPACE ADDR: %p\n", workspace);
   LOG_INFO("Initializing workspace");
}

void remove_workspace() {
   if (workspace) {
      free_readable_malloc(workspace, sizeof(*workspace));
      workspace = NULL;
   }
}
