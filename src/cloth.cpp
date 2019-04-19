#include <iostream>
#include <math.h>
#include <random>
#include <vector>

#include "cloth.h"
#include "collision/plane.h"
#include "collision/sphere.h"

using namespace std;

Cloth::Cloth(double width, double height, int num_width_points,
             int num_height_points, float thickness) {
  this->width = width;
  this->height = height;
  this->num_width_points = num_width_points;
  this->num_height_points = num_height_points;
  this->thickness = thickness;

  buildGrid();
  buildClothMesh();
}

Cloth::~Cloth() {
  point_masses.clear();
  springs.clear();

  if (clothMesh) {
    delete clothMesh;
  }
}

void Cloth::buildGrid() {
  // TODO (Part 1): Build a grid of masses and springs.
    //num_width_points by num_height_points total masses
    float w = width / num_width_points;
    float h = height / num_height_points;
    for (int i = 0 ; i < num_height_points ; i++){
        for (int j = 0 ; j < num_width_points ; j++){
            Vector3D pos;
            if (orientation == HORIZONTAL){
                pos = Vector3D(j*w, 1, i*h);
            }
            else{
                double offset = ((2.0* (double)rand() / (double) RAND_MAX) - 1.0) / 1000.0;
                pos = Vector3D(j*w, i*h, offset);
            }
            bool pin = false;
            for (int k = 0; k < pinned.size(); k++){
                if (pinned[k][0] == (j) && pinned[k][1] == (i)){
                    pin = true;
                }
            }
            PointMass m = PointMass(pos, pin);
            point_masses.push_back(m);
        }
    }
    
    for (int i = 0; i< point_masses.size(); i++){
        //structural constraints check
        PointMass* pm = &point_masses.at(i);
        if (i - num_width_points > 0){
            Spring structUP = Spring(pm, pm - num_width_points, STRUCTURAL);
            springs.push_back(structUP);
        }
        if ((i%num_width_points) > 0){
            Spring structLEFT = Spring(pm, pm - 1, STRUCTURAL);
            springs.push_back(structLEFT);
        }

        //shearing constraints check
        if ((i - num_width_points > 0) && ((i%num_width_points) > 0)){
            Spring shearLEFT = Spring(pm, pm-num_width_points-1, SHEARING);
            springs.push_back(shearLEFT);
        }
        if ((i - num_width_points > 0) && ((i%num_width_points) + 1 < num_width_points)){
            Spring shearRIGHT = Spring(pm, pm-num_width_points + 1, SHEARING);
            springs.push_back(shearRIGHT);
        }

        //bending constraints check
        if (i - (2*num_width_points) > 0){
            Spring bendUP = Spring(pm, pm-(2*num_width_points), BENDING);
            springs.push_back(bendUP);
        }
        if ((i%num_width_points) - 2 >= 0){
            Spring bendLEFT = Spring(pm, pm - 2, BENDING);
            springs.push_back(bendLEFT);
        }

    }
    
}

void Cloth::simulate(double frames_per_sec, double simulation_steps, ClothParameters *cp,
                     vector<Vector3D> external_accelerations,
                     vector<CollisionObject *> *collision_objects) {
  double mass = width * height * cp->density / num_width_points / num_height_points;
  double delta_t = 1.0f / frames_per_sec / simulation_steps;

  // TODO (Part 2): Compute total force acting on each point mass.
    for (int i = 0; i< point_masses.size(); i++){
        point_masses[i].forces = Vector3D(0,0,0);
    }
    
    Vector3D Ftotal;
    
    for (int i = 0; i<external_accelerations.size(); i++){
        Vector3D a = external_accelerations[i];
        Ftotal += mass * a;
    }
    for(int i = 0; i<point_masses.size(); i++){
        point_masses[i].forces += Ftotal;
    }
    
    
    //spring correction forces

    for (int i = 0; i< springs.size(); i++){
        Spring* s = &springs[i];
        if ((s->spring_type == BENDING and cp->enable_bending_constraints) or (s->spring_type == SHEARING and cp->enable_shearing_constraints) or (s->spring_type == STRUCTURAL and cp->enable_structural_constraints)){
            Vector3D pmA = s->pm_a->position;
            Vector3D pmB = s->pm_b->position;
            Vector3D mag = pmA - pmB;
            double ks = cp->ks;
            if (s->spring_type == BENDING){
                ks = ks * 0.2;
            }
            double rl = s->rest_length;
            Vector3D ForceSpring = ks * (mag.norm() - rl) * mag.unit();
            s->pm_a->forces -= ForceSpring;
            s->pm_b->forces += ForceSpring;
        }
    }
    

  // TODO (Part 2): Use Verlet integration to compute new point mass positions

        for (int i = 0; i<point_masses.size(); i++){
            Vector3D currposition = point_masses[i].position;
            Vector3D lastposition = point_masses[i].last_position;
            double d = cp->damping / 100.0;
            Vector3D a = point_masses[i].forces / mass;
            Vector3D newposition = currposition + ((1 - d) * (currposition - lastposition)) + (a * delta_t * delta_t);
        if (point_masses[i].pinned == false){
            point_masses[i].last_position = point_masses[i].position;
            point_masses[i].position = newposition;
        }
    }

  // TODO (Part 4): Handle self-collisions.
    build_spatial_map();
    for (int i = 0; i < point_masses.size(); i++){
        for (int j = 0; j < collision_objects->size(); j++){
            self_collide(point_masses[i], simulation_steps);
        }
    }

  // TODO (Part 3): Handle collisions with other primitives.
    for (int i = 0; i < point_masses.size(); i++){
        for (int j = 0; j < collision_objects->size(); j++){
            collision_objects->at(j)->collide(point_masses[i]);
        }
    }

  // TODO (Part 2): Constrain the changes to be such that the spring does not change
  // in length more than 10% per timestep [Provot 1995].
    
    for (int i = 0; i< springs.size(); i++){
        Spring* s = &springs[i];
        Vector3D pmA = s->pm_a->position;
        Vector3D pmB = s->pm_b->position;
        Vector3D mag = pmA - pmB;
        if (mag.norm() > 1.1 * s->rest_length){
            Vector3D x = s->rest_length * 1.1 * mag.unit();
            Vector3D dx = (1 - (x.norm() / mag.norm())) * mag / 2;
            if (s->pm_a->pinned==false && s->pm_b->pinned ==false){
                s->pm_a->position -= dx;
                s->pm_b->position += dx;
            }
            if (s->pm_a->pinned==true and s->pm_b->pinned==false){
                s->pm_b->position += 2.0 * dx;
            }
            if (s->pm_a->pinned==false and s->pm_b->pinned==true){
                s->pm_a->position -= 2.0 * dx;
            }
        }
    }

}

void Cloth::build_spatial_map() {
  for (const auto &entry : map) {
    delete(entry.second);
  }
  map.clear();

  // TODO (Part 4): Build a spatial map out of all of the point masses.
    for (int i = 0; i<point_masses.size(); i++){
        float hash = hash_position(point_masses[i].position);
        if (map.find(hash) == map.end()){
            vector<PointMass *>* v = new vector<PointMass *>();
            map[hash] = v;
        }
        vector<PointMass *>* pointer = map[hash];
        (*pointer).emplace_back(&point_masses[i]);
        
    }
}

void Cloth::self_collide(PointMass &pm, double simulation_steps) {
  // TODO (Part 4): Handle self-collision for a given point mass.
    float hash = hash_position(pm.position);
    vector<PointMass *>* pointmasses = map[hash];
    Vector3D finalCorrection;
    double pmCount = 0;
    for (int i = 0; i < pointmasses->size(); i++){
        if ((*pointmasses)[i] != &pm){
            Vector3D pos1 = pm.position;
            Vector3D pos2 = (*(*pointmasses)[i]).position;
            double dist = (pos1 - pos2).norm2();
            if (dist <= 2 * thickness * 2 * thickness){
                Vector3D dir = pos1 - pos2;
                dir.normalize();
                Vector3D correction = pos2 + (dir * 2 * thickness) - pos1;
                finalCorrection += correction;
                pmCount++;
            }
        }
    }
    if (pmCount > 0) pm.position += finalCorrection / (pmCount * simulation_steps);
    
}

float Cloth::hash_position(Vector3D pos) {
  // TODO (Part 4): Hash a 3D position into a unique float identifier that represents membership in some 3D box volume.
    float w = 3 * width / num_width_points;
    float h = 3 * height / num_height_points;
    float t = max(w, h);
    
    if(orientation == HORIZONTAL){
        //y / h, z / t
        return ((pos[0] / w) * 31) + ((pos[1] / h) * 31) + (pos[2] / t);
    }
    else{
        // z / h, y /t
        return (floor(pos[0] / w) * 31) + (floor(pos[1] / t) * 31) + floor(pos[2] / h);
    }
    
    
  return 0.f; 
}

///////////////////////////////////////////////////////
/// YOU DO NOT NEED TO REFER TO ANY CODE BELOW THIS ///
///////////////////////////////////////////////////////

void Cloth::reset() {
  PointMass *pm = &point_masses[0];
  for (int i = 0; i < point_masses.size(); i++) {
    pm->position = pm->start_position;
    pm->last_position = pm->start_position;
    pm++;
  }
}

void Cloth::buildClothMesh() {
  if (point_masses.size() == 0) return;

  ClothMesh *clothMesh = new ClothMesh();
  vector<Triangle *> triangles;

  // Create vector of triangles
  for (int y = 0; y < num_height_points - 1; y++) {
    for (int x = 0; x < num_width_points - 1; x++) {
      PointMass *pm = &point_masses[y * num_width_points + x];
      // Get neighboring point masses:
      /*                      *
       * pm_A -------- pm_B   *
       *             /        *
       *  |         /   |     *
       *  |        /    |     *
       *  |       /     |     *
       *  |      /      |     *
       *  |     /       |     *
       *  |    /        |     *
       *      /               *
       * pm_C -------- pm_D   *
       *                      *
       */
      
      float u_min = x;
      u_min /= num_width_points - 1;
      float u_max = x + 1;
      u_max /= num_width_points - 1;
      float v_min = y;
      v_min /= num_height_points - 1;
      float v_max = y + 1;
      v_max /= num_height_points - 1;
      
      PointMass *pm_A = pm                       ;
      PointMass *pm_B = pm                    + 1;
      PointMass *pm_C = pm + num_width_points    ;
      PointMass *pm_D = pm + num_width_points + 1;
      
      Vector3D uv_A = Vector3D(u_min, v_min, 0);
      Vector3D uv_B = Vector3D(u_max, v_min, 0);
      Vector3D uv_C = Vector3D(u_min, v_max, 0);
      Vector3D uv_D = Vector3D(u_max, v_max, 0);
      
      
      // Both triangles defined by vertices in counter-clockwise orientation
      triangles.push_back(new Triangle(pm_A, pm_C, pm_B, 
                                       uv_A, uv_C, uv_B));
      triangles.push_back(new Triangle(pm_B, pm_C, pm_D, 
                                       uv_B, uv_C, uv_D));
    }
  }

  // For each triangle in row-order, create 3 edges and 3 internal halfedges
  for (int i = 0; i < triangles.size(); i++) {
    Triangle *t = triangles[i];

    // Allocate new halfedges on heap
    Halfedge *h1 = new Halfedge();
    Halfedge *h2 = new Halfedge();
    Halfedge *h3 = new Halfedge();

    // Allocate new edges on heap
    Edge *e1 = new Edge();
    Edge *e2 = new Edge();
    Edge *e3 = new Edge();

    // Assign a halfedge pointer to the triangle
    t->halfedge = h1;

    // Assign halfedge pointers to point masses
    t->pm1->halfedge = h1;
    t->pm2->halfedge = h2;
    t->pm3->halfedge = h3;

    // Update all halfedge pointers
    h1->edge = e1;
    h1->next = h2;
    h1->pm = t->pm1;
    h1->triangle = t;

    h2->edge = e2;
    h2->next = h3;
    h2->pm = t->pm2;
    h2->triangle = t;

    h3->edge = e3;
    h3->next = h1;
    h3->pm = t->pm3;
    h3->triangle = t;
  }

  // Go back through the cloth mesh and link triangles together using halfedge
  // twin pointers

  // Convenient variables for math
  int num_height_tris = (num_height_points - 1) * 2;
  int num_width_tris = (num_width_points - 1) * 2;

  bool topLeft = true;
  for (int i = 0; i < triangles.size(); i++) {
    Triangle *t = triangles[i];

    if (topLeft) {
      // Get left triangle, if it exists
      if (i % num_width_tris != 0) { // Not a left-most triangle
        Triangle *temp = triangles[i - 1];
        t->pm1->halfedge->twin = temp->pm3->halfedge;
      } else {
        t->pm1->halfedge->twin = nullptr;
      }

      // Get triangle above, if it exists
      if (i >= num_width_tris) { // Not a top-most triangle
        Triangle *temp = triangles[i - num_width_tris + 1];
        t->pm3->halfedge->twin = temp->pm2->halfedge;
      } else {
        t->pm3->halfedge->twin = nullptr;
      }

      // Get triangle to bottom right; guaranteed to exist
      Triangle *temp = triangles[i + 1];
      t->pm2->halfedge->twin = temp->pm1->halfedge;
    } else {
      // Get right triangle, if it exists
      if (i % num_width_tris != num_width_tris - 1) { // Not a right-most triangle
        Triangle *temp = triangles[i + 1];
        t->pm3->halfedge->twin = temp->pm1->halfedge;
      } else {
        t->pm3->halfedge->twin = nullptr;
      }

      // Get triangle below, if it exists
      if (i + num_width_tris - 1 < 1.0f * num_width_tris * num_height_tris / 2.0f) { // Not a bottom-most triangle
        Triangle *temp = triangles[i + num_width_tris - 1];
        t->pm2->halfedge->twin = temp->pm3->halfedge;
      } else {
        t->pm2->halfedge->twin = nullptr;
      }

      // Get triangle to top left; guaranteed to exist
      Triangle *temp = triangles[i - 1];
      t->pm1->halfedge->twin = temp->pm2->halfedge;
    }

    topLeft = !topLeft;
  }

  clothMesh->triangles = triangles;
  this->clothMesh = clothMesh;
}
