#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/character_body3d.hpp>

using namespace godot;

class StairsCharacter3D : public CharacterBody3D
{
	GDCLASS(StairsCharacter3D, CharacterBody3D);

private:
	NodePath collider;
	double step_height = 0.5;

	bool grounded;
	bool was_grounded;

	Vector3 desired_velocity;
	Vector3 horizontal = Vector3(1,0,1);

	void set_collider(const NodePath &p_collider);
	NodePath get_collider() const;

protected:
	static void _bind_methods();

public:
	void _ready() override;
	void reset_grounded();

	void move_and_stair_step();
	void stair_step_up();
	void stair_step_down();

	void set_step_height(const double height) {
		step_height = height;
	}

	double get_step_height() const {
		return step_height;
	}

	void set_desired_velocity(const Vector3 vel) {
		desired_velocity = vel;
	}

	Vector3 get_desired_velocity() const {
		return desired_velocity;
	}

};
