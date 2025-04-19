#include "engine/hierarchy.h"
#include "raylib.h"
#include "typedef.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
   printf("addr: %p memory added: %zu memory capacity: %zu \n", ptr, len, memory_busy);
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
   char *newStr = malloc(len);
   if (newStr) {
      strcpy(newStr, str);
   }
   return newStr;
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
   node->variables = readable_malloc(8);
}

//
// === CHILD MANAGEMENT ===
//

void changeVoidChild(void ***childrenPtr, int *childCount) {
   size_t oldSize = sizeof(void *) * (*childCount);
   size_t newSize = sizeof(void *) * (*childCount + 1);
   void **newChildren = readable_realloc(*childrenPtr, newSize, oldSize);
   if (!newChildren) return;

   *childrenPtr = newChildren;
   (*childrenPtr)[*childCount] = *newChildren;
   *childCount += 1;
}

void childAdd__(void *parent, void *child) {
   if (!parent || !child) return;
baseNode *node = getBaseNode(parent);
changeVoidChild(&node->children, &node->childCount);
}

void removeChildrens(void ***children, int *childCount) {
   if (!children || !*children || !childCount || *childCount <= 0) return;
   for (int i = 0; i < *childCount; i++) {
      void *child = (*children)[i];
      if (!child) {
         LOG_WARN("removeChildrens: null child");
         continue;
      }
      kindStructType type = getIsKindType(child);
      if (type == PARENT_TYPE_OBJECT) {
         remove_object((objectSpace *)child);
      } else if (type == PARENT_TYPE_PART) {
         remove_shape((shapeProperties *)child);
      }
   }

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
   part->radius = isnan(radius) ? 1.0f : radius;

   LOG_INFO("partSphere: created");
   return part;
}

partCylinder *instance_cylinder(void *parent, shapeProperties *shape, float radiusTop, float radiusBottom, float height, int slices) {
   partCylinder *part = readable_malloc(sizeof(partCylinder));
   if (!part) return NULL;

   shape->part = part;
   shape->partType = OBJECT_TYPE_CYLINDER;

   part->radiusTop = isnan(radiusTop) ? 1.0f : radiusTop;
   part->radiusBottom = isnan(radiusBottom) ? 1.0f : radiusBottom;
   part->height = isnan(height) ? 1.0f : height;
   part->slices = (slices <= 0) ? 1 : slices;

   LOG_INFO("partCylinder: created");
   return part;
}

partMesh *instance_meshPart(void *parent, shapeProperties *shape, Mesh mesh, Texture2D texture, char *meshPath) {
   partMesh *part = readable_malloc(sizeof(partMesh));
   if (!part) return NULL;

   shape->part = part;
   shape->partType = OBJECT_TYPE_MESHPART;

   part->mesh = mesh;
   part->texture = texture;
   part->meshPath = meshPath ? str_dup(meshPath) : NULL;

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
   switch (type) {
      case OBJECT_TYPE_CUBE:
         instance_cube(actualParent, newShape);
         return newShape;
      case OBJECT_TYPE_SPHERE:
         instance_sphere(actualParent, newShape, ((partSphere *)params)->radius);
         return newShape;
      case OBJECT_TYPE_CYLINDER: 
         {
            partCylinder *p = (partCylinder *)params;
            instance_cylinder(actualParent, newShape, p->radiusTop, p->radiusBottom, p->height, p->slices);
            return newShape;
         }
      case OBJECT_TYPE_MESHPART: 
         {
            partMesh *p = (partMesh *)params;
            instance_meshPart(actualParent, newShape, p->mesh, p->texture, p->meshPath);
            return newShape;
         }
      default:
         remove_shape(newShape);
         return NULL;
   }
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
   node->parent = NULL;

   if (node->variables) {
      free_readable_malloc(node->variables, sizeof(node->variables));
      node->variables = NULL;
   }

   if (shape->part) {
      removePart(shape->part, shape->partType);
      shape->part = NULL;
   }

   removeChildrens(&node->children, &node->childCount);
   free_readable_malloc(shape, sizeof(*shape));

}

void childDestroy(void *ptr) {
   if (!ptr) return;
   kindStructType kindtype = getIsKindType(ptr);
   switch (kindtype) {
      case PARENT_TYPE_OBJECT:
         LOG_INFO("removing PARENT_TYPE_OBJECT");
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

   return object;
}

void remove_object(objectSpace *object) {
   if (!object) return;
   baseNode *node = getBaseNode(object);
   removeChildrens(&node->children, &node->childCount);
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
   
   shape->anchored = false;
   shape->castShadow = true;
   shape->transparency = 0;
   shape->size = (Vector3){2.0, 2.0, 2.0};
   shape->orientation = (Vector3){0.0, 0.0, 0.0};
   shape->material = MATERIAL_TYPE_PLASTIC;
   shape->brickColor = (Color){127, 127, 127, 255};
   shape->collisionGroup = COLLISION_GROUP_ONE;

   return 0;
}

//
// === WORKSPACE ===
//

void init_workspace() {
   remove_workspace();

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
