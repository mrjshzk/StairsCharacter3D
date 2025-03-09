#include "StairsCharacter.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/physics_test_motion_parameters3d.hpp>
#include <godot_cpp/classes/physics_test_motion_result3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>

using namespace godot;

void StairsCharacter3D::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_collider"), &StairsCharacter3D::get_collider);
	ClassDB::bind_method(D_METHOD("set_collider", "collider"), &StairsCharacter3D::set_collider);
	ClassDB::add_property("StairsCharacter3D", PropertyInfo(Variant::NODE_PATH, "collider", PROPERTY_HINT_NODE_TYPE, "CollisionShape3D"), "set_collider", "get_collider");

	ClassDB::bind_method(D_METHOD("get_step_height_up"), &StairsCharacter3D::get_step_height_up);
	ClassDB::bind_method(D_METHOD("set_step_height_up", "height"), &StairsCharacter3D::set_step_height_up);

	ClassDB::bind_method(D_METHOD("get_step_height_down"), &StairsCharacter3D::get_step_height_down);
	ClassDB::bind_method(D_METHOD("set_step_height_down", "height"), &StairsCharacter3D::set_step_height_down);

	ClassDB::add_property("StairsCharacter3D", PropertyInfo(Variant::FLOAT, "step_height_up", PROPERTY_HINT_RANGE, "0,5,0.05"), "set_step_height_up", "get_step_height_up");
	ClassDB::add_property("StairsCharacter3D", PropertyInfo(Variant::FLOAT, "step_height_down", PROPERTY_HINT_RANGE, "0,5,0.05"), "set_step_height_down", "get_step_height_down");

	ClassDB::bind_method(D_METHOD("move_and_stair_step"), &StairsCharacter3D::move_and_stair_step);

	ADD_SIGNAL(MethodInfo("on_stair_step_up"));
	ADD_SIGNAL(MethodInfo("on_stair_step_down"));
	ADD_SIGNAL(MethodInfo("on_stair_step"));
}

void StairsCharacter3D::_ready() 
{
	
	Node *col = get_node_or_null(collider);
	if (col == nullptr) {
		UtilityFunctions::push_error("Collider is null");
	}
}

void StairsCharacter3D::reset_grounded() 
{
	was_grounded = grounded;
	grounded = is_on_floor();
	desired_velocity = Vector3(0,0,0);
}



void StairsCharacter3D::move_and_stair_step()
{
	reset_grounded();
	set_desired_velocity(get_velocity().normalized());
	stair_step_up();
	move_and_slide();
	stair_step_down();
}

void StairsCharacter3D::stair_step_up()
{
	if (grounded == false) {
		return;
	}

	Vector3 horizontal_velocity = horizontal * get_velocity();
	Vector3 testing_velocity = desired_velocity;
	if (horizontal_velocity != Vector3(0,0,0)) {
		testing_velocity = horizontal_velocity;
	}

	// Not moving or attempting to move, skip stair check
	if (testing_velocity == Vector3(0,0,0)) {
		return;
	}

	Ref<PhysicsTestMotionResult3D> result = memnew(PhysicsTestMotionResult3D);
	Ref<PhysicsTestMotionParameters3D> params = memnew(PhysicsTestMotionParameters3D);

	params->set_margin(0.01);

	// This variable gets reused for all the following checks
	Transform3D motion_transform = get_global_transform();

	Vector3 distance = testing_velocity * get_physics_process_delta_time();
	params->set_from(motion_transform);
	params->set_motion(distance);
	
	// No stair step to do, we didn't hit any walls
	if (PhysicsServer3D::get_singleton()->body_test_motion(get_rid(), params, result) == false) {
		return;
	}

	// Move to collision
	Vector3 remainder = result->get_remainder();
	motion_transform = motion_transform.translated(result->get_travel());

	// Raise up to ceiling - can't walk on steps if there's a low ceiling
	Vector3 step_up = step_height_up * Vector3(0,1,0);
	

	params->set_from(motion_transform);
	params->set_motion(step_up);
	PhysicsServer3D::get_singleton()->body_test_motion(get_rid(), params, result);


	// GetTravel will be full length if we didn't hit anything
	motion_transform = motion_transform.translated(result->get_travel());
	float step_up_distance = result->get_travel().length();

	// Move forward remaining distance
	params->set_from(motion_transform);
	params->set_motion(remainder);
	PhysicsServer3D::get_singleton()->body_test_motion(get_rid(), params, result);
	motion_transform = motion_transform.translated(result->get_travel());

	// And set the collider back down again
	params->set_from(motion_transform);
	// But no further than how far we stepped up
	params->set_motion(Vector3(0, -1, 0) * step_up_distance);

	// Don't bother with the rest if we're not actually gonna land back down on something
	if (PhysicsServer3D::get_singleton()->body_test_motion(get_rid(), params, result) == false) {
		return;
	}
	
	motion_transform = motion_transform.translated(result->get_travel());

	Vector3 surface_normal = result->get_collision_normal();

	// Can't stand on the thing we're trying to step on anyway
	if (surface_normal.angle_to(Vector3(0, 1, 0)) > get_floor_max_angle()) {
		return;
	}

	// Move player to match the step height we just found
	set_global_position(Vector3(get_global_position().x, motion_transform.origin.y, get_global_position().z));
	emit_signal("on_stair_step");
	emit_signal("on_stair_step_up");
}

void StairsCharacter3D::stair_step_down()
{
	if (was_grounded == false || get_velocity().y >= 0)
	{
		return;
	}

	Ref<PhysicsTestMotionResult3D> result = memnew(PhysicsTestMotionResult3D);
	Ref<PhysicsTestMotionParameters3D> params = memnew(PhysicsTestMotionParameters3D);

	params->set_from(get_global_transform());
	params->set_motion(Vector3(0,-1,0) * step_height_down);
	params->set_margin(0.01);

	if (PhysicsServer3D::get_singleton()->body_test_motion(get_rid(), params, result) == false) {
		return;
	}

	set_global_transform(get_global_transform().translated(result->get_travel()));
	apply_floor_snap();
	emit_signal("on_stair_step");
	emit_signal("on_stair_step_down");
}

void StairsCharacter3D::set_collider(const NodePath &p_collider)
{
	collider = p_collider;
}

NodePath StairsCharacter3D::get_collider() const 
{
	return collider;
}