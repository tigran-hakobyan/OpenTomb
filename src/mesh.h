
#ifndef MESH_H
#define MESH_H

#define MESH_FULL_OPAQUE      0x00  // Fully opaque object (all polygons are opaque: all t.flags < 0x02)
#define MESH_HAS_TRANSPARENCY 0x01  // Fully transparency or has transparency and opaque polygon / object

#define ANIM_CMD_MOVE               0x01
#define ANIM_CMD_CHANGE_DIRECTION   0x02
#define ANIM_CMD_JUMP               0x04

#define COLLISION_NONE              (0x00)
#define COLLISION_TRIMESH           (0x01)
#define COLLISION_BOX               (0x02)
#define COLLISION_SPHERE            (0x03)
#define COLLISION_BASE_BOX          (0x04)


#include <SDL2/SDL_platform.h>
#include <SDL2/SDL_opengl.h>
#include <cstdint>
#include <bullet/LinearMath/btScalar.h>
#include "vertex_array.h"
#include "object.h"
#include <memory>
#include <vector>
#include "vmath.h"

class btCollisionShape;
class btRigidBody;
class btCollisionShape;

struct polygon_s;
struct Room;
struct EngineContainer;
struct obb_s;
struct vertex_s;
struct Entity;

typedef struct transparent_polygon_reference_s {
    const struct polygon_s *polygon;
    vertex_array *used_vertex_array;
    unsigned firstIndex;
    unsigned count;
    bool isAnimated;
} transparent_polygon_reference_t, *transparent_polygon_reference_p;

/*
 * Animated version of vertex. Does not contain texture coordinate, because that is in a different VBO.
 */
typedef struct animated_vertex_s {
    btVector3 position;
    std::array<float,4> color;
    btVector3 normal;
} animated_vertex_t, *animated_vertex_p;

/*
 * base mesh, uses everywhere
 */
typedef struct base_mesh_s
{
    uint32_t              id;                                                   // mesh's ID
    uint8_t               uses_vertex_colors;                                   // does this mesh have prebaked vertex lighting

    std::vector<polygon_s> polygons;                                             // polygons data

    struct polygon_s     *transparency_polygons;                                // transparency mesh's polygons list

    uint32_t              num_texture_pages;                                    // face without structure wrapping
    uint32_t             *element_count_per_texture;                            //
    uint32_t             *elements;                                             //
    uint32_t alpha_elements;

    std::vector<vertex_s> vertices;
    
    uint32_t num_animated_elements;
    uint32_t num_alpha_animated_elements;
    uint32_t *animated_elements;
    uint32_t animated_vertex_count;
    struct animated_vertex_s *animated_vertices;
    
    uint32_t transparent_polygon_count;
    struct transparent_polygon_reference_s *transparent_polygons;

    btVector3 centre;                                            // geometry centre of mesh
    btVector3 bb_min;                                            // AABB bounding volume
    btVector3 bb_max;                                            // AABB bounding volume
    btScalar              R;                                                    // radius of the bounding sphere
    int8_t               *matrix_indices;                                       // vertices map for skin mesh

    GLuint                vbo_vertex_array;
    GLuint                vbo_index_array;
    GLuint                vbo_skin_array;
    vertex_array *        main_vertex_array;
    
    // Buffers for animated polygons
    // The first contains position, normal and color.
    // The second contains the texture coordinates. It gets updated every frame.
    GLuint                animated_vbo_vertex_array;
    GLuint                animated_vbo_texcoord_array;
    GLuint                animated_vbo_index_array;
    vertex_array *        animated_vertex_array;
}base_mesh_t, *base_mesh_p;


/*
 * base sprite structure
 */
typedef struct sprite_s
{
    uint32_t            id;                                                     // object's ID
    uint32_t            texture;                                                // texture number
    GLfloat             tex_coord[8];                                           // texture coordinates
    uint32_t            flag;
    btScalar            left;                                                   // world sprite's gabarites
    btScalar            right;
    btScalar            top;
    btScalar            bottom;
}sprite_t, *sprite_p;

/*
 * Structure for all the sprites in a room
 */
typedef struct sprite_buffer_s
{
    // Vertex data for the sprites
    vertex_array *data;
    
    // How many sub-ranges the element_array_buffer contains. It has one for each texture listed.
    uint32_t              num_texture_pages;
    // The element count for each sub-range.
    uint32_t             *element_count_per_texture;
}sprite_buffer_t, *sprite_buffer_p;

/*
 * lights
 */
enum LightType
{
    LT_NULL,
    LT_POINT,
    LT_SPOTLIGHT,
    LT_SUN,
    LT_SHADOW
};


typedef struct light_s
{
    btVector3 pos;                                         // world position
    float                       colour[4];                                      // RGBA value

    float                       inner;
    float                       outer;
    float                       length;
    float                       cutoff;

    float                       falloff;

    LightType                   light_type;
}light_t, *light_p;

/*
 *  Animated sequence. Used globally with animated textures to refer its parameters and frame numbers.
 */

typedef struct tex_frame_s
{
    btScalar    mat[4];
    btScalar    move[2];
    uint16_t    tex_ind;
}tex_frame_t, *tex_frame_p;

typedef struct anim_seq_s
{
    bool        uvrotate;               // UVRotate mode flag.
    bool        frame_lock;             // Single frame mode. Needed for TR4-5 compatible UVRotate.

    bool        blend;                  // Blend flag.  Reserved for future use!
    btScalar    blend_rate;             // Blend rate.  Reserved for future use!
    btScalar    blend_time;             // Blend value. Reserved for future use!

    int8_t      anim_type;              // 0 = normal, 1 = back, 2 = reverse.
    bool        reverse_direction;      // Used only with type 2 to identify current animation direction.
    btScalar    frame_time;             // Time passed since last frame update.
    uint16_t    current_frame;          // Current frame for this sequence.
    btScalar    frame_rate;             // For types 0-1, specifies framerate, for type 3, should specify rotation speed.
    uint16_t    frames_count;           // Overall frames to use. If type is 3, it should be 1, else behaviour is undetermined.

    btScalar    uvrotate_speed;         // Speed of UVRotation, in seconds.
    btScalar    uvrotate_max;           // Reference value used to restart rotation.
    btScalar    current_uvrotate;       // Current coordinate window position.

    struct tex_frame_s  *frames;

    uint32_t*   frame_list;       // Offset into anim textures frame list.
}anim_seq_t, *anim_seq_p;


/*
 * room static mesh.
 */
struct StaticMesh : public Object
{
    uint32_t                    object_id;                                      //
    uint8_t                     was_rendered;                                   // 0 - was not rendered, 1 - opaque, 2 - transparency, 3 - full rendered
    uint8_t                     was_rendered_lines;
    uint8_t                     hide;                                           // disable static mesh rendering
    btVector3 pos;                                         // model position
    btVector3 rot;                                         // model angles
    std::array<float,4> tint;                                        // model tint

    btVector3 vbb_min;                                     // visible bounding box
    btVector3 vbb_max;
    btVector3 cbb_min;                                     // collision bounding box
    btVector3 cbb_max;

    btTransform transform;                                  // gl transformation matrix
    obb_s               *obb;
    EngineContainer  *self;

    base_mesh_s         *mesh;                                           // base model
    btRigidBody                *bt_body;
};

/*
 * Animated skeletal model. Taken from openraider.
 * model -> animation -> frame -> bone
 * thanks to Terry 'Mongoose' Hendrix II
 */

/*
 * SMOOTHED ANIMATIONS STRUCTURES
 * stack matrices are needed for skinned mesh transformations.
 */
typedef struct ss_bone_tag_s
{
    struct ss_bone_tag_s   *parent;
    uint16_t                index;
    base_mesh_p             mesh_base;                                          // base mesh - pointer to the first mesh in array
    base_mesh_p             mesh_skin;                                          // base skinned mesh for ТР4+
    base_mesh_p             mesh_slot;
    btVector3 offset;                                          // model position offset

    btQuaternion qrotate;                                         // quaternion rotation
    btTransform transform;    // 4x4 OpenGL matrix for stack usage
    btTransform full_transform;    // 4x4 OpenGL matrix for global usage

    uint32_t                body_part;                                          // flag: BODY, LEFT_LEG_1, RIGHT_HAND_2, HEAD...
}ss_bone_tag_t, *ss_bone_tag_p;


typedef struct ss_animation_s
{
    int16_t                     last_state;
    int16_t                     next_state;
    int16_t                     last_animation;
    int16_t                     current_animation;                              //
    int16_t                     next_animation;                                 //
    int16_t                     current_frame;                                  //
    int16_t                     next_frame;                                     //

    uint16_t                    anim_flags;                                     // additional animation control param

    btScalar                    period;                                         // one frame change period
    btScalar                    frame_time;                                     // current time
    btScalar                    lerp;

    void                      (*onFrame)(std::shared_ptr<Entity> ent, struct ss_animation_s *ss_anim, int state);

    struct skeletal_model_s    *model;                                          // pointer to the base model
    struct ss_animation_s      *next;
}ss_animation_t, *ss_animation_p;

/*
 * base frame of animated skeletal model
 */
typedef struct ss_bone_frame_s
{
    uint16_t                    bone_tag_count;                                 // number of bones
    struct ss_bone_tag_s       *bone_tags;                                      // array of bones
    btVector3 pos;                                         // position (base offset)
    btVector3 bb_min;                                      // bounding box min coordinates
    btVector3 bb_max;                                      // bounding box max coordinates
    btVector3 centre;                                      // bounding box centre

    struct ss_animation_s       animations;                                     // animations list
    
    bool hasSkin;                                       // whether any skinned meshes need rendering
}ss_bone_frame_t, *ss_bone_frame_p;

/*
 * ORIGINAL ANIMATIONS
 */
typedef struct bone_tag_s
{
    btVector3 offset;                                            // bone vector
    btQuaternion qrotate;                                           // rotation quaternion
}bone_tag_t, *bone_tag_p;

/*
 * base frame of animated skeletal model
 */
typedef struct bone_frame_s
{
    uint16_t            bone_tag_count;                                         // number of bones
    uint16_t            command;                                                // & 0x01 - move need, &0x02 - 180 rotate need
    struct bone_tag_s  *bone_tags;                                              // bones data
    btVector3 pos;                                                 // position (base offset)
    btVector3 bb_min;                                              // bounding box min coordinates
    btVector3 bb_max;                                              // bounding box max coordinates
    btVector3 centre;                                              // bounding box centre
    btVector3 move;                                                // move command data
    btScalar            v_Vertical;                                             // jump command data
    btScalar            v_Horizontal;                                           // jump command data
}bone_frame_t, *bone_frame_p ;

/*
 * mesh tree base element structure
 */
typedef struct mesh_tree_tag_s
{
    base_mesh_p                 mesh_base;                                      // base mesh - pointer to the first mesh in array
    base_mesh_p                 mesh_skin;                                      // base skinned mesh for ТР4+
    btVector3 offset;                                      // model position offset
    uint16_t                    flag;                                           // 0x0001 = POP, 0x0002 = PUSH, 0x0003 = RESET
    uint32_t                    body_part;
    uint8_t                     replace_mesh;                                   // flag for shoot / guns animations (0x00, 0x01, 0x02, 0x03)
    uint8_t                     replace_anim;
}mesh_tree_tag_t, *mesh_tree_tag_p;

/*
 * animation switching control structure
 */
typedef struct anim_dispatch_s
{
    uint16_t    next_anim;                                                      // "switch to" animation
    uint16_t    next_frame;                                                     // "switch to" frame
    uint16_t    frame_low;                                                      // low border of state change condition
    uint16_t    frame_high;                                                     // high border of state change condition
}anim_dispatch_t, *anim_dispatch_p;

typedef struct state_change_s
{
    uint32_t                    id;
    uint16_t                    anim_dispatch_count;
    struct anim_dispatch_s     *anim_dispatch;
}state_change_t, *state_change_p;

/*
 * one animation frame structure
 */
typedef struct animation_frame_s
{
    uint32_t                    id;
    uint8_t                     original_frame_rate;
    btScalar                    speed_x;                // Forward-backward speed
    btScalar                    accel_x;                // Forward-backward accel
    btScalar                    speed_y;                // Left-right speed
    btScalar                    accel_y;                // Left-right accel
    uint32_t                    anim_command;
    uint32_t                    num_anim_commands;
    uint16_t                    state_id;
    uint16_t                    frames_count;           // Number of frames
    struct bone_frame_s        *frames;                 // Frame data

    uint16_t                    state_change_count;     // Number of animation statechanges
    struct state_change_s      *state_change;           // Animation statechanges data

    struct animation_frame_s   *next_anim;              // Next default animation
    int                         next_frame;             // Next default frame
}animation_frame_t, *animation_frame_p;

/*
 * skeletal model with animations data.
 */

typedef struct skeletal_model_s
{
    uint32_t                    id;                                             // ID
    uint8_t                     transparency_flags;                             // transparancy flags; 0 - opaque; 1 - alpha test; other - blending mode
    uint8_t                     hide;                                           // do not render
    btVector3 bbox_min;                                    // bbox info
    btVector3 bbox_max;
    btVector3 centre;                                      // the centre of model

    uint16_t                    animation_count;                                // number of animations
    struct animation_frame_s   *animations;                                     // animations data

    uint16_t                    mesh_count;                                     // number of model meshes
    struct mesh_tree_tag_s     *mesh_tree;                                      // base mesh tree.
    uint16_t                    collision_map_size;
    uint16_t                   *collision_map;
}skeletal_model_t, *skeletal_model_p;


void BaseMesh_Clear(base_mesh_p mesh);
void BaseMesh_FindBB(base_mesh_p mesh);
void Mesh_GenVBO(const struct render_s *renderer, struct base_mesh_s *mesh);

void SkeletalModel_Clear(skeletal_model_p model);
void SkeletonModel_FillTransparency(skeletal_model_p model);
void SkeletalModel_InterpolateFrames(skeletal_model_p models);
void FillSkinnedMeshMap(skeletal_model_p model);

void SSBoneFrame_CreateFromModel(ss_bone_frame_p bf, skeletal_model_p model);

void BoneFrame_Copy(bone_frame_p dst, bone_frame_p src);
mesh_tree_tag_p SkeletonClone(mesh_tree_tag_p src, int tags_count);
void SkeletonCopyMeshes(mesh_tree_tag_p dst, mesh_tree_tag_p src, int tags_count);
void SkeletonCopyMeshes2(mesh_tree_tag_p dst, mesh_tree_tag_p src, int tags_count);


uint32_t Mesh_AddVertex(base_mesh_p mesh, const vertex_s &vertex);
void Mesh_GenFaces(base_mesh_p mesh);

/* bullet collision model calculation */
btCollisionShape* BT_CSfromBBox(const btVector3 &bb_min, const btVector3 &bb_max, bool useCompression, bool buildBvh, bool is_static);
btCollisionShape* BT_CSfromMesh(struct base_mesh_s *mesh, bool useCompression, bool buildBvh, bool is_static = true);
btCollisionShape* BT_CSfromHeightmap(struct room_sector_s *heightmap, struct sector_tween_s *tweens, int tweens_size, bool useCompression, bool buildBvh);

#endif
