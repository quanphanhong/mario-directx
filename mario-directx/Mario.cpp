#include <algorithm>
#include <assert.h>
#include "Utils.h"

#include "Mario.h"
#include "Game.h"

#include "Goomba.h"
#include "Portal.h"
#include "Brick.h"
#include "ColoredBlock.h"
#include "Coin.h"
#include "Koopa.h"
#include "QuestionBrick.h"
#include "Mushroom.h"
#include "SwitchBlock.h"
#include "Bullet.h"
#include "TubeEnemy.h"
#include "Leaf.h"
#include "Tube.h"
#include "GroundBricks.h"
#include "SquareBrick.h"
#include "Camera.h"
#include "Score.h"
#include "HUD.h"
#include "Reward.h"
#include "FloatingBlock.h"
#include "Boomerang.h"
#include "BoomerangBro.h"
#include "FireFlower.h"

bool CMario::UpdateDyingDelay()
{
	if (dying_delay)
	{
		if ((DWORD)GetTickCount64() - dying_delay_start > MARIO_DIE_DELAY)
		{
			dying_delay = 0;
			return false;
		}
		return true;
	}
	return false;
}

void CMario::CheckReleasingKoopa()
{
	// Mario does not hold Koopa
	if (holdenKoopa && !isInIntro)
	{
		if (holdenKoopa->GetState() != KOOPA_STATE_LYING_DOWN && holdenKoopa->GetState() != KOOPA_STATE_LYING_UP)
			releaseKoopa();
	}
}

void CMario::UpdateSparkling()
{
	if (sparkle && (DWORD)GetTickCount64() - sparkle_start > MARIO_TAIL_SPARKLE_TIME)
	{
		sparkle = 0;
		sparkle_start = 0;
	}
}

void CMario::UpdateMarioPassingLevel()
{
	// When Mario has passed current level, let him run to the right
	if (passedTheLevel)
	{
		SetState(MARIO_STATE_RUNNING_RIGHT);
		keyFacing = MARIO_FACING_RIGHT;
	}
}

bool CMario::UpdateMarioLevelTransformation()
{
	if (levelTransform)
	{
		if (GetTickCount64() - stepStart > MARIO_LEVEL_TRANSFORMING_TIME)
		{
			// switch between two levels
			if (level == transform_lastLevel)
				SetLevel(transform_newLevel);
			else
				SetLevel(transform_lastLevel);

			transformSteps++;
			stepStart = (DWORD)GetTickCount64();
		}

		// Transforming has finished
		if (transformSteps >= MARIO_LEVEL_TRANSFORMING_STEPS)
		{
			transformSteps = 0;
			SetLevel(transform_newLevel);
			levelTransform = 0;
		}
		return true;
	}

	return false;
}

bool CMario::UpdateMarioSwitchingZone(DWORD dt)
{
	// Update Mario flying when he is up/down the tube
	if (flyingDirection != FLYING_DIRECTION_NOMOVE)
	{
		UpdateFlying(dt);
		if (flyingDirection == FLYING_DIRECTION_NOMOVE)
			allowSwichingZone = 1;
		return true;
	}

	return false;
}

void CMario::UpdateMarioJumpingState()
{
	float _lastVy = vy;

	if (jumping) running = 0;

	if (jumpingUp && _lastVy <= 0.0f) vy = -MARIO_JUMP_SPEED_Y;
	if (fly == MARIO_FLYING_STATE_NONE)
	{
		if (level == MARIO_LEVEL_TAIL)
		{
			if (flyJump && _lastVy >= 0.0f)
			{
				if (!jumping)
					flyJump = 0;
				vy -= MARIO_FLY_JUMP_SPEED_Y;
			}

			if (flyJump && (DWORD)GetTickCount64() - flyJump_start > MARIO_FLY_JUMP_TIME)
			{
				flyJump = 0;
				flyJump_start = 0;
			}
		}
		else
		{
			if (jumpMaxPower)
			{
				if (jumpMaxPower == MARIO_JUMP_MAX_POWER_UP)
					vy -= MARIO_JUMP_MAX_POWER_SPEED_Y;
				
				if (jumpMaxPower == MARIO_JUMP_MAX_POWER_UP &&
					(DWORD)GetTickCount64() - jump_max_power_start > MARIO_JUMP_MAX_POWER_TIME)
				{
					jump_max_power_start = 0;
					jumpMaxPower = MARIO_JUMP_MAX_POWER_DOWN;
				}

				if (!jumping) jumpMaxPower = MARIO_JUMP_MAX_POWER_NONE;
			}
		}
	}
	else if (fly == MARIO_FLYING_STATE_UP)
	{
		if (flyJump)
		{
			if (!jumping)
			{
				flyJump = 0;
				fly = MARIO_FLYING_STATE_NONE;
			}
			vy = (float)-MARIO_FLY_SPEED_Y;
		}

		if ((DWORD)GetTickCount64() - flyJump_start > MARIO_FLY_TIME ||
			(DWORD)GetTickCount64() - fly_clicking_start > MARIO_FLY_CLICKING_TIME)
		{
			SetJumpingUp(0);
			flyJump = 0;
			fly = MARIO_FLYING_STATE_DOWN;
		}
	}
	else
	{
		if (!jumping)
		{
			flyJump = 0;
			fly = MARIO_FLYING_STATE_NONE;
		}
	}

	if (jumpingUp && !flyJump && !fly)
	{
		// Mario can Jump for a specific height
		if (lastStandingHeight - this->y > MARIO_JUMP_HEIGHT)
			SetJumpingUp(0);
	}
}

void CMario::CheckMarioRunningCondition()
{
	if (!lastRunning && running)
		StartRunning();
}

void CMario::UpdateRunningState()
{
	DWORD runningTime = (DWORD)GetTickCount64() - running_start;

	if (running)
	{
		// set Power Level for displaying and identify flying ability of Mario
		powerLevel = (int)(((float)runningTime / MARIO_RUNNING_TIME) * MAXIMUM_POWER_LEVEL);
		powerLevel = (powerLevel > MAXIMUM_POWER_LEVEL) ? MAXIMUM_POWER_LEVEL : (int)powerLevel;
	}
	else if (fly != MARIO_FLYING_STATE_UP) ReducePowerLevel();
}

void CMario::CheckAndSetMagicWings()
{
	if (gainedMagicWings) powerLevel = MAXIMUM_POWER_LEVEL;
}

void CMario::CheckMarioTurning()
{
	// Check if Mario has stopped turning process
	if (turning)
	{
		if (vx == 0.0f) turning = 0;
	}
}

void CMario::UpdateMarioSpeed(DWORD dt)
{
	CheckMarioTurning();
	UpdateMarioJumpingState();
	CheckMarioRunningCondition();
	UpdateRunningState();

	if (state == MARIO_STATE_DIE) vx = 0.0f;

	// Simple fall down
	vy += MARIO_GRAVITY;

	// Calculate dx, dy 
	CGameObject::Update(dt);
}

void CMario::CheckMarioThrowingFireballs()
{
	// Fire Mario throws fireballs
	if (throwing && (DWORD)GetTickCount64() - throwing_start > MARIO_THROW_TIME)
	{
		throwing = 0;
		throwing_start = 0;
		ThrowFireball();
	}
}

void CMario::CheckMarioKickingState()
{
	// Mario stops kicking Koopa
	if (GetTickCount64() - kicking_start > MARIO_KICKING_TIME)
	{
		kicking = 0;
		kicking_start = 0;
	}
}

void CMario::CheckMarioSpinningState()
{
	// when Mario is attacking using his tail
	if (spinning)
	{
		spinningTime = (DWORD)GetTickCount64();
		DWORD spinning_time = spinningTime - spinning_start;
		int body_spinning = MARIO_TAIL_SPINNING_WIDTH - MARIO_TAIL_SPINNING_LENGTH;
		int body_facing = MARIO_TAIL_FACING_SCREEN_WIDTH;

		if (state == MARIO_STATE_IDLE)
			CCamera::GetInstance()->LockCamera(1);
		else
		{
			if (nx > 0) vx = MARIO_SPINNING_SPEED_X;
			else vx = -MARIO_SPINNING_SPEED_X;
			CCamera::GetInstance()->LockCamera(0);
		}

		if (spinning_time > MARIO_SPINNING_TIME)
		{
			// spinning phase 2
			if (spinningPhase == 1)
			{
				if (nx > 0)
				{
					x += MARIO_TAIL_SPINNING_LENGTH;
				}
				else
				{
					x += body_spinning - body_facing;
				}

				spinningPhase++;
				hittableTail = 0;
			}
			// spinning phase 3
			else if (spinningPhase == 2)
			{
				hittableTail = 1;
				if (nx < 0)
				{
					x -= (body_spinning - body_facing + MARIO_TAIL_SPINNING_LENGTH);
					SetTail(x, x + MARIO_TAIL_SPINNING_LENGTH);
				}
				else SetTail(x + MARIO_TAIL_SPINNING_WIDTH - MARIO_TAIL_SPINNING_LENGTH, x + MARIO_TAIL_SPINNING_WIDTH);
				spinningPhase++;
			}
			// spinning phase 4
			else if (spinningPhase == 3)
			{
				if (nx < 0)
				{
					x += MARIO_TAIL_SPINNING_LENGTH + body_spinning - body_facing;
				}
				spinningPhase++;
				hittableTail = 0;
			}
			// spinning phase 5
			else if (spinningPhase == 4)
			{
				if (nx > 0)
				{
					x -= MARIO_TAIL_SPINNING_LENGTH;
					SetTail(x, x + MARIO_TAIL_SPINNING_LENGTH);
				}
				else
				{
					x -= body_spinning - body_facing;
					SetTail(x + MARIO_TAIL_SPINNING_WIDTH - MARIO_TAIL_SPINNING_LENGTH, x + MARIO_TAIL_SPINNING_WIDTH);
				}
				spinningPhase++;
				hittableTail = 1;
			}
			// spinning phase 6
			else
			{
				spinning = 0;
				spinning_start = 0;
				hittableTail = 0;
				if (nx > 0)
					x += MARIO_TAIL_SPINNING_LENGTH - MARIO_TAIL_NORMAL_LENGTH;
			}
			spinning_start = (DWORD)GetTickCount64();
		}
	}
	else CCamera::GetInstance()->LockCamera(0);
}

void CMario::CheckMarioUntouchableState()
{
	// reset untouchable timer if untouchable time has passed
	if ((DWORD)GetTickCount64() - untouchable_start > MARIO_UNTOUCHABLE_TIME)
	{
		untouchable_start = 0;
		untouchable = 0;
		background = 0;
	}
}

void CMario::UpdateMarioCollision(vector<LPCOLLISIONEVENT> coEvents, vector<LPGAMEOBJECT>* coObjects, float lastX, float lastY, float lastVx, float lastVy)
{
	vector<LPCOLLISIONEVENT> coEventsResult;
	coEvents.clear();

	// turn off collision when Mario is dead 
	if (state != MARIO_STATE_DIE)
		CalcPotentialCollisions(coObjects, coEvents);

	// No collision occured, proceed normally
	if (coEvents.size() == 0)
	{
		jumping = 1;
		x += dx;
		y += dy;
	}
	else
	{
		float min_tx, min_ty, nx = 0, ny;
		float rdx = 0;
		float rdy = 0;

		FilterCollision(coEvents, coEventsResult, min_tx, min_ty, nx, ny, rdx, rdy);

		// block every object first!
		x += min_tx * dx + nx * 0.08f;
		y += min_ty * dy + ny * 0.08f;

		if (nx != 0) vx = 0;
		if (ny != 0) vy = 0;

		vector<LPCOLLISIONEVENT> floorList;
		floorList.clear();

		//
		// Collision logic with other objects
		//
		jumping = 1;
		int acquiredQuestionBrick = 0;
		int keepMoving = 0;
		for (UINT i = 0; i < coEventsResult.size(); i++)
		{
			LPCOLLISIONEVENT e = coEventsResult[i];

			float obj_x, obj_y;
			e->obj->GetPosition(obj_x, obj_y);

			float obj_l, obj_t, obj_r, obj_b;
			e->obj->GetBoundingBox(obj_l, obj_t, obj_r, obj_b);

			float obj_w = obj_r - obj_l;
			float obj_h = obj_b - obj_t;

			int canBeHitBySpinning = (e->nx != 0 && spinning && obj_y <= y + MARIO_TAIL_HEAD_TO_TAIL &&
				obj_y + obj_h >= y + MARIO_TAIL_HEAD_TO_TAIL + MARIO_TAIL_TAIL_WIDTH) ? 1 : 0;

			CGame* game = CGame::GetInstance();

			if (e->ny < 0)
			{
				jumping = 0;
				floorList.push_back(e);
				// reset score streak
				if (dynamic_cast<CQuestionBrick*>(e->obj) || dynamic_cast<CBrick*>(e->obj) ||
					dynamic_cast<CGroundBricks*>(e->obj) || dynamic_cast<CTube*>(e->obj) ||
					dynamic_cast<CSquareBrick*>(e->obj)) scoreStreak = 0;
			}

			if (e->ny > 0)
			{
				// ignore disallowing jumping up when Mario collides with Coin, Leaf, Mushroom and Reward
				if (!dynamic_cast<CCoin*>(e->obj) || dynamic_cast<CLeaf*>(e->obj) || dynamic_cast<CMushroom*>(e->obj) ||
					dynamic_cast<CReward*>(e->obj) || dynamic_cast<CFloatingBlock*>(e->obj))
				{
					fly = 0;
					jumpingUp = 0;
				}
			}
			
			if (dynamic_cast<CGoomba*>(e->obj))
			{
				CGoomba* goomba = dynamic_cast<CGoomba*>(e->obj);
				if (e->ny < 0)
				{
					if (goomba->GetState() != GOOMBA_STATE_DIE && goomba->GetState() != GOOMBA_STATE_DIE_AND_FLY)
					{
						AddStreakScore(goomba);
						goomba->LevelDown();
						vy = -MARIO_JUMP_DEFLECT_SPEED;
					}
				}
				else if (e->nx != 0)
				{
					if (canBeHitBySpinning)
					{
						goomba->HitGoomba(nx);
						SetSparkling();
					}
					else if (untouchable == 0)
					{
						if (goomba->GetState() != GOOMBA_STATE_DIE)
						{
							LevelDown();
						}
					}
				}
				keepMoving = 1;
			}
			else if (dynamic_cast<CKoopa*>(e->obj))
			{
				CKoopa* koopa = dynamic_cast<CKoopa*>(e->obj);

				int koopaState = koopa->GetState();

				if (e->ny != 0)
				{
					if (e->ny < 0)
					{
						AddStreakScore(koopa);
						if (koopaState == KOOPA_STATE_JUMPING_LEFT || koopaState == KOOPA_STATE_JUMPING_RIGHT ||
							koopaState == KOOPA_STATE_UP || koopaState == KOOPA_STATE_DOWN)
							koopa->LevelDown();
						else if (koopaState == KOOPA_STATE_WALKING_LEFT || koopaState == KOOPA_STATE_WALKING_RIGHT)
							koopa->SetState(KOOPA_STATE_LYING_DOWN);
						else if (koopaState == KOOPA_STATE_ROLLING_DOWN_LEFT || koopaState == KOOPA_STATE_ROLLING_DOWN_RIGHT)
							koopa->SetState(KOOPA_STATE_LYING_DOWN);
						else if (koopaState == KOOPA_STATE_ROLLING_UP_LEFT || koopaState == KOOPA_STATE_ROLLING_UP_RIGHT)
							koopa->SetState(KOOPA_STATE_LYING_UP);
						vy = -MARIO_JUMP_DEFLECT_SPEED;
					}
					else if (e->ny > 0)
					{
						if (isInIntro)
						{
							continue;
						}
						if (koopaState != KOOPA_STATE_LYING_UP && koopaState != KOOPA_STATE_LYING_DOWN)
						{
							keepMoving = 1;
							LevelDown();
						}
					}

					if (koopaState == KOOPA_STATE_LYING_DOWN || koopaState == KOOPA_STATE_LYING_UP)
					{
						float left, top, right, bottom;
						GetBoundingBox(left, top, right, bottom);

						keepMoving = 1;

						float koopa_x, koopa_y;
						koopa->GetPosition(koopa_x, koopa_y);

						if (koopa_x > x)
						{
							nx = 1;
							if (!allowHodingKoopa) StartKicking();
							if (koopaState == KOOPA_STATE_LYING_DOWN)
								koopa->SetState(KOOPA_STATE_ROLLING_DOWN_RIGHT);
							else
								koopa->SetState(KOOPA_STATE_ROLLING_UP_RIGHT);
						}
						else
						{
							nx = -1;
							if (!allowHodingKoopa) StartKicking();
							if (koopaState == KOOPA_STATE_LYING_DOWN)
								koopa->SetState(KOOPA_STATE_ROLLING_DOWN_LEFT);
							else
								koopa->SetState(KOOPA_STATE_ROLLING_UP_LEFT);
						}
						continue;
					}
				}
				else if (e->nx != 0)
				{
					if (canBeHitBySpinning)
					{
						koopa->HitKoopa((int)nx);
						AddStreakScore(koopa);
						SetSparkling();
					}
					else if ((koopaState == KOOPA_STATE_LYING_UP || koopaState == KOOPA_STATE_LYING_DOWN) &&
						(state == MARIO_STATE_RUNNING_RIGHT || state == MARIO_STATE_RUNNING_LEFT ||
							state == MARIO_STATE_RUNNING_FAST_RIGHT || state == MARIO_STATE_RUNNING_FAST_LEFT))
					{
						setHoldenKoopa(koopa);
					}
					else if (koopaState == KOOPA_STATE_LYING_DOWN)
					{
						if (!allowHodingKoopa) StartKicking();
						if (e->nx < 0)
							koopa->SetState(KOOPA_STATE_ROLLING_DOWN_RIGHT);
						else
							koopa->SetState(KOOPA_STATE_ROLLING_DOWN_LEFT);
						AddStreakScore(koopa);
					}
					else if (koopaState == KOOPA_STATE_LYING_UP)
					{
						if (!allowHodingKoopa) StartKicking();
						if (e->nx < 0)
							koopa->SetState(KOOPA_STATE_ROLLING_UP_RIGHT);
						else
							koopa->SetState(KOOPA_STATE_ROLLING_UP_LEFT);
						AddStreakScore(koopa);
					}
					else
					{
						if (!untouchable) LevelDown();

						keepMoving = 1;
					}
				}
			}
			else if (dynamic_cast<CBoomerangBro*>(e->obj))
			{
				CBoomerangBro* boomerangBro = dynamic_cast<CBoomerangBro*>(e->obj);

				if (boomerangBro->GetState() == BOOMERANG_BRO_STATE_DYING)
				{
					keepMoving = 1;
					continue;
				}

				if (e->ny < 0)
					boomerangBro->SetState(BOOMERANG_BRO_STATE_DYING);
				else
					LevelDown();

				keepMoving = 1;
			}
			else if (dynamic_cast<CQuestionBrick*>(e->obj) && !acquiredQuestionBrick)
			{
				float l, t, b, r;
				GetBoundingBox(l, t, r, b);

				float width = r - l, height = b - t;

				CQuestionBrick* brick = dynamic_cast<CQuestionBrick*>(e->obj);
				int result = 0;
				if (e->ny > 0)
				{
					if ((x <= obj_x && x + width - obj_x >= width / 2.5f) || (x >= obj_x && obj_x + width - x >= width / 2.5f))
						brick->HitQuestionBrick((l > obj_x) ? 1 : -1);

					continue;
				}
				if (canBeHitBySpinning)
				{
					result = brick->HitQuestionBrick(this->nx);
					SetSparkling();
				}

				if (result) acquiredQuestionBrick = 1;
			}
			else if (dynamic_cast<CPortal*>(e->obj))
			{
				CPortal* p = dynamic_cast<CPortal*>(e->obj);
				CGame::GetInstance()->SwitchScene(p->GetSceneId());
				for (UINT i = 0; i < coEvents.size(); i++) delete coEvents[i];
				return;
			}
			else if (dynamic_cast<CBullet*>(e->obj) || dynamic_cast<CBoomerang*>(e->obj) || dynamic_cast<CTubeEnemy*>(e->obj))
			{
				if (!untouchable)
					this->LevelDown();

				keepMoving = 1;
			}
			else if (dynamic_cast<CMushroom*>(e->obj))
			{
				CMushroom* mushroom = dynamic_cast<CMushroom*>(e->obj);
				mushroom->Gain(this);

				keepMoving = 1;
			}
			else if (CFireFlower* fireFlower = dynamic_cast<CFireFlower*>(e->obj))
			{
				fireFlower->SetState(FIRE_FLOWER_STATE_UNAVAILABLE);
				TurnIntoFire();
				keepMoving = 1;
			}
			else if (dynamic_cast<CLeaf*>(e->obj))
			{
				CLeaf* leaf = dynamic_cast<CLeaf*>(e->obj);
				this->LevelUp();
				leaf->SetState(LEAF_STATE_UNAVAILABLE);

				keepMoving = 1;
			}
			else if (dynamic_cast<CTube*>(e->obj))
			{
				CTube* tube = dynamic_cast<CTube*>(e->obj);

				int lidType = tube->GetLidType();
				int zoneToSwitch = tube->GetZoneToSwitch();
				if (zoneToSwitch != TUBE_ZONE_NO_DOOR) {
					if ((lidType == TUBE_TYPE_UPPER_LID && e->ny < 0 && readyToDown) || (lidType == TUBE_TYPE_LOWER_LID && e->ny > 0 && readyToUp))
					{
						float switch_x, switch_y;
						tube->GetSwitchingPosition(switch_x, switch_y);
						CGame::GetInstance()->ChangePlayZone(zoneToSwitch, switch_x, switch_y);
						StartSwitchingZone((readyToUp) ? MARIO_SWITCHING_ZONE_DIRECTION_UP : MARIO_SWITCHING_ZONE_DIRECTION_DOWN);
					}
				}
			}
			else if (dynamic_cast<CSquareBrick*>(e->obj))
			{
				CSquareBrick* squareBrick = dynamic_cast<CSquareBrick*>(e->obj);

				if (canBeHitBySpinning || e->ny > 0)
				{
					squareBrick->Destroy();
					SetSparkling();
				}
			}
			else if (dynamic_cast<CReward*>(e->obj))
			{
				CReward* reward = dynamic_cast<CReward*>(e->obj);
				reward->Gain();
				passedTheLevel = 1;
				keepMoving = 1;
			}
			else if (dynamic_cast<CSwitchBlock*>(e->obj))
			{
				CSwitchBlock* switchBlock = dynamic_cast<CSwitchBlock*>(e->obj);
				if (e->ny < 0) switchBlock->Switch();
			}
			else if (dynamic_cast<CFloatingBlock*>(e->obj))
			{
				CFloatingBlock* floatingBlock = dynamic_cast<CFloatingBlock*>(e->obj);

				if (e->ny < 0)
				{
					// floating block start dropping
					if (floatingBlock->GetState() != FLOATING_BLOCK_STATE_DROP)
						floatingBlock->SetState(FLOATING_BLOCK_STATE_DROP);
				}
			}
		}

		// If Mario is allowed to continue moving, set his speed back to previous value
		if (keepMoving)
		{
			x = lastX + dx;
			vx = lastVx;

			if (jumping || lastVy < 0)
			{
				y = lastY + dy;
				vy = lastVy;
			}
		}

		float min_t = 1.0f;
		LPCOLLISIONEVENT min_event = NULL;
		// Set the nearest object to Mario as floor
		for (LPCOLLISIONEVENT event : floorList)
		{
			if (event->t < min_t)
			{
				if (min_event)
				{
					if (dynamic_cast<CFloatingBlock*>(event->obj) &&
						(dynamic_cast<CBrick*>(min_event->obj) ||
							dynamic_cast<CQuestionBrick*>(min_event->obj) ||
							dynamic_cast<CSquareBrick*>(min_event->obj)))
						continue;
				}

				floor = event->obj;
				min_t = event->t;
				min_event = event;
			}
		}
	}
}

void CMario::FixFloor()
{
	if (jumping)
	{
		if (floor)
		{
			if (dynamic_cast<CFloatingBlock*>(floor))
			{
				float fx, fy;
				floor->GetPosition(fx, fy);

				float l, t, r, b;
				GetBoundingBox(l, t, r, b);

				float cx, cy;
				CCamera::GetInstance()->GetPosition(cx, cy);

				float sw, sh;
				sw = (float)CGame::GetInstance()->GetScreenWidth();
				sh = (float)GAME_PLAY_HEIGHT;

				float fl, ft, fr, fb;
				floor->GetBoundingBox(fl, ft, fr, fb);

				// when Mario is outside floating block, remove the floor that he is marked
				if (fx < cx || fy < cy || fx > cx + sw || fy > cy + sh || x + (r - l) < fx || x > fx + (fr - fl) || jumpingUp)
					floor = NULL;
				// on the other hand, he is standing -> which means he is not jumping
				else jumping = 0;
			}
			else floor = NULL;
		}
	}

	if (floor)
	{
		if (jumping) floor = NULL;
		else
		{
			float floor_x, floor_y;
			floor->GetPosition(floor_x, floor_y);
			float left, top, right, bottom;
			GetBoundingBox(left, top, right, bottom);
			float height = bottom - top;

			if (y > floor_y - height)
				y = floor_y - height;
			else if (y < floor_y - height && !jumpingUp && dynamic_cast<CFloatingBlock*>(floor))
				y = floor_y - height;
		}
	}
}

void CMario::SetKoopaPosition()
{
	if (holdenKoopa != NULL)
	{
		float l, t, r, b;
		GetBoundingBox(l, t, r, b);

		if (keyFacing == MARIO_FACING_RIGHT)
			holdenKoopa->SetPosition(r + HOLDEN_KOOPA_OFFSET_RIGHT_X, b + HOLDEN_KOOPA_OFFSET_RIGHT_Y);
		else
			holdenKoopa->SetPosition(l + HOLDEN_KOOPA_OFFSET_LEFT_X, b + HOLDEN_KOOPA_OFFSET_LEFT_Y);

		holdenKoopa->SetSpeed(vx, vy);
	}
}

void CMario::CleanUpCollisionEvents(vector<LPCOLLISIONEVENT> coEvents)
{
	// clean up collision events
	for (UINT i = 0; i < coEvents.size(); i++) delete coEvents[i];
}

void CMario::GetSparklePosition(float &x, float &y)
{
	if ((keyFacing == MARIO_FACING_LEFT && (spinningPhase == 1 || spinningPhase == 5)) ||
		(keyFacing == MARIO_FACING_RIGHT && spinningPhase == 2))
	{
		x = this->x + MARIO_TAIL_SPARKLE_OFFSET_LEFT_X;
		y = this->y + MARIO_TAIL_SPARKLE_OFFSET_LEFT_Y;
	}
	else
	{
		x = this->x + MARIO_TAIL_SPARKLE_OFFSET_RIGHT_X;
		y = this->y + MARIO_TAIL_SPARKLE_OFFSET_RIGHT_Y;
	}
}

CMario::CMario(float x, float y) : CGameObject()
{
	level = MARIO_LEVEL_SMALL;
	levelTransform = 0;
	untouchable = 0;
	turning = 0;
	kicking = 0;
	keyFacing = MARIO_FACING_RIGHT;

	holdenKoopa = NULL;

	SetState(MARIO_STATE_IDLE);

	flyingDirection = lastFlyingDirection = FLYING_DIRECTION_NOMOVE;

	jumping = 0;
	background = 0;

	gainedMagicWings = 0;

	running = 0;
	lastRunning = 0;
	allowSwichingZone = 0;

	scoreStreak = 0;
	isInIntro = 0;

	start_x = x; 
	start_y = y; 
	this->x = x; 
	this->y = y;
}

void CMario::Update(DWORD dt, vector<LPGAMEOBJECT> *coObjects)
{
	if (state == MARIO_STATE_UNAVAILABLE) return;

	if (UpdateDyingDelay()) return;

	float lastX = x, lastY = y;
	float lastVx = vx, lastVy = vy;

	UpdateMarioPassingLevel();
	CheckReleasingKoopa();
	UpdateSparkling();
	CheckAndSetMagicWings();

	if (UpdateMarioLevelTransformation() || UpdateMarioSwitchingZone(dt)) return;
	
	// Update Mario moving speed
	UpdateMarioSpeed(dt);
	
	// Check and update Mario special states
	CheckMarioThrowingFireballs();
	CheckMarioKickingState();
	CheckMarioSpinningState();
	CheckMarioUntouchableState();

	vector<LPCOLLISIONEVENT> coEvents;
	UpdateMarioCollision(coEvents, coObjects, lastX, lastY, lastVx, lastVy);
	FixFloor();
	SetKoopaPosition();

	CleanUpCollisionEvents(coEvents);
}

void CMario::SetMovingLeft(int skillButtonPressed)
{
	keyFacing = MARIO_FACING_LEFT;
	DWORD runningTime = (DWORD)GetTickCount64() - running_start;

	if (skillButtonPressed)
	{
		if (runningTime > MARIO_RUNNING_TIME)
			SetState(MARIO_STATE_RUNNING_FAST_LEFT);
		else
			SetState(MARIO_STATE_RUNNING_LEFT);
	}
	else
		SetState(MARIO_STATE_WALKING_LEFT);

	if (spinning) vx = -MARIO_SPINNING_SPEED_X;
	else if (fly) vx = GetMarioExpectedSpeedX(-MARIO_FLYING_SPEED_X);
	else if (flyJump) vx = GetMarioExpectedSpeedX(-MARIO_FLYING_JUMP_SPEED_X);
}

void CMario::SetMovingRight(int skillButtonPressed)
{
	keyFacing = MARIO_FACING_RIGHT;
	DWORD runningTime = (DWORD)GetTickCount64() - running_start;

	if (skillButtonPressed)
	{
		if (runningTime > MARIO_RUNNING_TIME)
			SetState(MARIO_STATE_RUNNING_FAST_RIGHT);
		else
			SetState(MARIO_STATE_RUNNING_RIGHT);
	}
	else
		SetState(MARIO_STATE_WALKING_RIGHT);

	if (spinning) vx = MARIO_SPINNING_SPEED_X;
	else if (fly) vx = GetMarioExpectedSpeedX(MARIO_FLYING_SPEED_X);
	else if (flyJump) vx = GetMarioExpectedSpeedX(MARIO_FLYING_JUMP_SPEED_X);
}

void CMario::Render()
{
	if (state == MARIO_STATE_UNAVAILABLE) return;

	int res = -1;

	// Mario turns into smoke when transforming from/into Racoon level
	if (levelTransform && (transform_lastLevel == MARIO_LEVEL_TAIL || transform_newLevel == MARIO_LEVEL_TAIL) && !isInIntro)
		res = MARIO_ANI_TAIL_TRANSFORM;
	else if (levelTransform && transform_newLevel == MARIO_LEVEL_TAIL)
		res = MARIO_ANI_TAIL_TRANSFORM;
	// Render based on current level of Mario
	else if (level == MARIO_LEVEL_SMALL) res = RenderSmallMario();
	else if (level == MARIO_LEVEL_BIG) res = RenderBigMario();
	else if (level == MARIO_LEVEL_TAIL) res = RenderTailMario();
	else if (level == MARIO_LEVEL_FIRE) res = RenderFireMario();
	else if (level == MARIO_LEVEL_LUIGI) res = RenderLuigi();

	if (sparkle)
		animation_set->at(MARIO_ANI_SPARKLE)->Render(sparkle_x, sparkle_y);

	renderAlpha = MARIO_UNTOUCHABLE_ALPHA_HIGH;
	if (untouchable)
	{
		if ((DWORD)GetTickCount64() - alpha_switch_start > MARIO_UNTOUCHABLE_CHANGING_ALPHA_TIME)
		{
			if (renderAlpha == MARIO_UNTOUCHABLE_ALPHA_HIGH)
				renderAlpha = MARIO_UNTOUCHABLE_ALPHA_LOW;
			else
				renderAlpha = MARIO_UNTOUCHABLE_ALPHA_HIGH;
			alpha_switch_start = (DWORD)GetTickCount64();
		}
	}

	if (res != -1)
	{
		if (level == MARIO_LEVEL_TAIL && turning && !spinning && !holdenKoopa)
			animation_set->at(res)->Render(x, y - MARIO_TAIL_TURNING_OFFSET_Y, renderAlpha);
		else
			animation_set->at(res)->Render(x, y, renderAlpha);
	}
}

int CMario::RenderSmallMario()
{
	int res = -1;

	if (flyingDirection == FLYING_DIRECTION_UP || flyingDirection == FLYING_DIRECTION_DOWN)
		res = MARIO_ANI_SMALL_SWICHING_SCENE;
	else if (levelTransform)
	{
		if (nx > 0) res = MARIO_ANI_SMALL_IDLE_RIGHT;
		else res = MARIO_ANI_SMALL_IDLE_LEFT;
	}
	else if (turning && !holdenKoopa && !jumping)
	{
		if (vx > 0) res = MARIO_ANI_SMALL_TURNING_LEFT;
		else res = MARIO_ANI_SMALL_TURNING_RIGHT;
	}
	else if (state == MARIO_STATE_DIE)
		res = MARIO_ANI_DIE;
	else
	{
		if (vx == 0)
		{
			if (keyFacing == MARIO_FACING_RIGHT)
			{
				if (jumping)
				{
					if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_JUMPING_RIGHT;
					else if (jumpMaxPower) res = MARIO_SMALL_JUMP_MAX_POWER_RIGHT;
					else res = MARIO_ANI_SMALL_JUMPING_RIGHT;
				}
				else if (kicking) res = MARIO_ANI_SMALL_KICKING_RIGHT;
				else
				{
					if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_IDLE_RIGHT;
					else res = MARIO_ANI_SMALL_IDLE_RIGHT;
				}
			}
			else
			{
				if (jumping)
				{
					if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_JUMPING_LEFT;
					else if (jumpMaxPower) res = MARIO_SMALL_JUMP_MAX_POWER_LEFT;
					else res = MARIO_ANI_SMALL_JUMPING_LEFT;
				}
				else if (kicking) res = MARIO_ANI_SMALL_KICKING_LEFT;
				else
				{
					if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_IDLE_LEFT;
					else res = MARIO_ANI_SMALL_IDLE_LEFT;
				}
			}
		}
		else if (keyFacing == MARIO_FACING_RIGHT)
		{
			if (jumping)
			{
				if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_JUMPING_RIGHT;
				else if (jumpMaxPower) res = MARIO_SMALL_JUMP_MAX_POWER_RIGHT;
				else res = MARIO_ANI_SMALL_JUMPING_RIGHT;

			}
			else if (kicking) res = MARIO_ANI_SMALL_KICKING_RIGHT;
			else if (vx * nx <= MARIO_WALKING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_WALKING_RIGHT;
				else res = MARIO_ANI_SMALL_WALKING_RIGHT;
			}
			else if (vx * nx <= MARIO_RUNNING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_RUNNING_RIGHT;
				else res = MARIO_ANI_SMALL_RUNNING_RIGHT;
			}
			else
			{
				if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_RUNNING_RIGHT;
				else res = MARIO_ANI_SMALL_RUNNING_FASTER_RIGHT;
			}
		}
		else if (keyFacing == MARIO_FACING_LEFT)
		{
			if (jumping)
			{
				if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_JUMPING_LEFT;
				else if (jumpMaxPower) res = MARIO_SMALL_JUMP_MAX_POWER_LEFT;
				else res = MARIO_ANI_SMALL_JUMPING_LEFT;

			}
			else if (kicking) res = MARIO_ANI_SMALL_KICKING_LEFT;
			else if (vx * nx <= MARIO_WALKING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_WALKING_LEFT;
				else res = MARIO_ANI_SMALL_WALKING_LEFT;
			}
			else if (vx * nx <= MARIO_RUNNING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_RUNNING_LEFT;
				else res = MARIO_ANI_SMALL_RUNNING_LEFT;
			}
			else
			{
				if (holdenKoopa) res = MARIO_ANI_SMALL_HOLD_KOOPA_RUNNING_LEFT;
				else res = MARIO_ANI_SMALL_RUNNING_FASTER_LEFT;
			}
		}
	}
	return res;
}

int CMario::RenderBigMario()
{
	int res = -1;

	if (state == MARIO_STATE_JUMPING_OUT)
		res = MARIO_ANI_INTRO_JUMPING_OUT;
	else if (state == MARIO_STATE_LOOKING_UP)
		res = MARIO_ANI_INTRO_LOOKING_UP;
	else if (flyingDirection == FLYING_DIRECTION_UP || flyingDirection == FLYING_DIRECTION_DOWN)
		res = MARIO_ANI_BIG_SWITCHING_SCENE;
	else if (levelTransform)
	{
		if (nx > 0) res = MARIO_ANI_BIG_IDLE_RIGHT;
		else res = MARIO_ANI_BIG_IDLE_LEFT;
	}
	else if (sitting)
	{
		if (nx > 0) res = MARIO_ANI_BIG_SITTING_RIGHT;
		else res = MARIO_ANI_BIG_SITTING_LEFT;
	}
	else if (turning && !holdenKoopa && !jumping)
	{
		if (vx > 0) res = MARIO_ANI_BIG_TURNING_LEFT;
		else res = MARIO_ANI_BIG_TURNING_RIGHT;
	}
	else
	{
		if (vx == 0)
		{
			if (keyFacing == MARIO_FACING_RIGHT)
			{
				if (jumping)
				{
					if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_JUMPING_RIGHT;
					else if (jumpMaxPower) res = MARIO_BIG_JUMP_MAX_POWER_RIGHT;
					else res = MARIO_ANI_BIG_JUMPING_RIGHT;
				}
				else if (kicking) res = MARIO_ANI_BIG_KICKING_RIGHT;
				else
				{
					if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_IDLE_RIGHT;
					else res = MARIO_ANI_BIG_IDLE_RIGHT;
				}
			}
			else
			{
				if (jumping)
				{
					if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_JUMPING_LEFT;
					else if (jumpMaxPower) res = MARIO_BIG_JUMP_MAX_POWER_LEFT;
					else res = MARIO_ANI_BIG_JUMPING_LEFT;
				}
				else if (kicking) res = MARIO_ANI_BIG_KICKING_LEFT;
				else
				{
					if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_IDLE_LEFT;
					else res = MARIO_ANI_BIG_IDLE_LEFT;
				}
			}
		}
		else if (keyFacing == MARIO_FACING_RIGHT)
		{
			if (jumping)
			{
				if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_JUMPING_RIGHT;
				else if (jumpMaxPower) res = MARIO_BIG_JUMP_MAX_POWER_RIGHT;
				else res = MARIO_ANI_BIG_JUMPING_RIGHT;

			}
			else if (kicking) res = MARIO_ANI_BIG_KICKING_RIGHT;
			else if (vx * nx <= MARIO_WALKING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_WALKING_RIGHT;
				else res = MARIO_ANI_BIG_WALKING_RIGHT;
			}
			else if (vx * nx <= MARIO_RUNNING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_RUNNING_RIGHT;
				else res = MARIO_ANI_BIG_RUNNING_RIGHT;
			}
			else
			{
				if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_RUNNING_RIGHT;
				else res = MARIO_ANI_BIG_RUNNING_FASTER_RIGHT;
			}
		}
		else if (keyFacing == MARIO_FACING_LEFT)
		{
			if (jumping)
			{
				if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_JUMPING_LEFT;
				else if (jumpMaxPower) res = MARIO_BIG_JUMP_MAX_POWER_LEFT;
				else res = MARIO_ANI_BIG_JUMPING_LEFT;

			}
			else if (kicking) res = MARIO_ANI_BIG_KICKING_LEFT;
			else if (vx * nx <= MARIO_WALKING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_WALKING_LEFT;
				else res = MARIO_ANI_BIG_WALKING_LEFT;
			}
			else if (vx * nx <= MARIO_RUNNING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_RUNNING_LEFT;
				else res = MARIO_ANI_BIG_RUNNING_LEFT;
			}
			else
			{
				if (holdenKoopa) res = MARIO_ANI_BIG_HOLD_KOOPA_RUNNING_LEFT;
				else res = MARIO_ANI_BIG_RUNNING_FASTER_LEFT;
			}
		}
	}
	return res;
}

int CMario::RenderTailMario()
{
	int res = -1;

	if (flyingDirection == FLYING_DIRECTION_UP || flyingDirection == FLYING_DIRECTION_DOWN)
		res = MARIO_ANI_TAIL_SWITCHING_SCENE;
	else if (spinning)
	{
		if (nx > 0)
		{
			switch (spinningPhase)
			{
			case 1:
				res = MARIO_ANI_TAIL_ATTACK_LEFT;
				break;
			case 2:
				res = MARIO_ANI_FACING_THE_SCREEN;
				break;
			case 3:
				res = MARIO_ANI_TAIL_ATTACK_RIGHT;
				break;
			case 4:
				res = MARIO_ANI_FACING_DOWN_THE_SCREEN;
				break;
			case 5:
				res = MARIO_ANI_TAIL_ATTACK_LEFT;
				break;
			}
		}
		else
		{
			switch (spinningPhase)
			{
			case 1:
				res = MARIO_ANI_TAIL_ATTACK_RIGHT;
				break;
			case 2:
				res = MARIO_ANI_FACING_THE_SCREEN;
				break;
			case 3:
				res = MARIO_ANI_TAIL_ATTACK_LEFT;
				break;
			case 4:
				res = MARIO_ANI_FACING_DOWN_THE_SCREEN;
				break;
			case 5:
				res = MARIO_ANI_TAIL_ATTACK_RIGHT;
				break;
			}
		}
	}
	else if (sitting)
	{
		if (nx > 0) res = MARIO_ANI_TAIL_SITTING_RIGHT;
		else MARIO_ANI_TAIL_SITTING_LEFT;
	}
	else if (turning && !holdenKoopa && !jumping)
	{
		if (vx > 0) res = MARIO_ANI_TAIL_TURNING_LEFT;
		else res = MARIO_ANI_TAIL_TURNING_RIGHT;
	}
	else
	{
		if (vx == 0)
		{
			if (keyFacing == MARIO_FACING_RIGHT)
			{
				if (jumping)
				{
					if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_JUMPING_RIGHT;
					else if (vy < 0)
					{
						if (fly) res = MARIO_ANI_TAIL_FLY_RIGHT;
						else if (flyJump) res = MARIO_ANI_TAIL_FLY_JUMP_RIGHT;
						else res = MARIO_ANI_TAIL_JUMPING_RIGHT;
					}
					else
					{
						if (fly) res = MARIO_ANI_TAIL_FLY_DROP_RIGHT;
						else if (flyJump) res = MARIO_ANI_TAIL_FLY_JUMP_RIGHT;
						else res = MARIO_ANI_TAIL_FLY_JUMP_DROP_RIGHT;
					}
				}
				else if (kicking) res = MARIO_ANI_TAIL_KICKING_RIGHT;
				else
				{
					if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_IDLE_RIGHT;
					else res = MARIO_ANI_TAIL_IDLE_RIGHT;
				}
			}
			else
			{
				if (jumping)
				{
					if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_JUMPING_LEFT;
					else if (vy < 0)
					{
						if (fly) res = MARIO_ANI_TAIL_FLY_LEFT;
						else if (flyJump) res = MARIO_ANI_TAIL_FLY_JUMP_LEFT;
						else res = MARIO_ANI_TAIL_JUMPING_LEFT;
					}
					else
					{
						if (fly) res = MARIO_ANI_TAIL_FLY_DROP_LEFT;
						else if (flyJump) res = MARIO_ANI_TAIL_FLY_JUMP_LEFT;
						else res = MARIO_ANI_TAIL_FLY_JUMP_DROP_LEFT;
					}
				}
				else if (kicking) res = MARIO_ANI_TAIL_KICKING_LEFT;
				else
				{
					if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_IDLE_LEFT;
					else res = MARIO_ANI_TAIL_IDLE_LEFT;
				}
			}
		}
		else if (keyFacing == MARIO_FACING_RIGHT)
		{
			if (jumping)
			{
				if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_JUMPING_RIGHT;
				else if (vy < 0)
				{
					if (fly)
					{
						if (keyFacing == MARIO_FACING_RIGHT)
							res = MARIO_ANI_TAIL_FLY_RIGHT;
						else
							res = MARIO_ANI_TAIL_FLY_LEFT;
					}
					else res = MARIO_ANI_TAIL_JUMPING_RIGHT;
				}
				else
				{
					if (fly) res = MARIO_ANI_TAIL_FLY_DROP_RIGHT;
					else if (flyJump) res = MARIO_ANI_TAIL_FLY_JUMP_RIGHT;
					else res = MARIO_ANI_TAIL_FLY_JUMP_DROP_RIGHT;
				}
			}
			else if (kicking) res = MARIO_ANI_TAIL_KICKING_RIGHT;
			else if (vx * nx <= MARIO_WALKING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_WALKING_RIGHT;
				else res = MARIO_ANI_TAIL_WALKING_RIGHT;
			}
			else if (vx * nx <= MARIO_RUNNING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_RUNNING_RIGHT;
				else res = MARIO_ANI_TAIL_RUNNING_RIGHT;
			}
			else
			{
				if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_RUNNING_RIGHT;
				else res = MARIO_ANI_TAIL_RUNNING_FASTER_RIGHT;
			}
		}
		else
		{
			if (jumping)
			{
				if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_JUMPING_LEFT;
				else if (vy < 0)
				{
					if (fly)
					{
						if (keyFacing == MARIO_FACING_RIGHT)
							res = MARIO_ANI_TAIL_FLY_RIGHT;
						else
							res = MARIO_ANI_TAIL_FLY_LEFT;
					}
					else res = MARIO_ANI_TAIL_JUMPING_LEFT;
				}
				else
				{
					if (fly) res = MARIO_ANI_TAIL_FLY_DROP_LEFT;
					else if (flyJump) res = MARIO_ANI_TAIL_FLY_JUMP_LEFT;
					else res = MARIO_ANI_TAIL_FLY_JUMP_DROP_LEFT;
				}
			}
			else if (kicking) res = MARIO_ANI_TAIL_KICKING_LEFT;
			else if (vx * nx <= MARIO_WALKING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_WALKING_LEFT;
				else res = MARIO_ANI_TAIL_WALKING_LEFT;
			}
			else if (vx * nx <= MARIO_RUNNING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_RUNNING_LEFT;
				else res = MARIO_ANI_TAIL_RUNNING_LEFT;
			}
			else
			{
				if (holdenKoopa) res = MARIO_ANI_TAIL_HOLD_KOOPA_RUNNING_LEFT;
				else res = MARIO_ANI_TAIL_RUNNING_FASTER_LEFT;
			}
		}
	}
	return res;
}

int CMario::RenderFireMario()
{
	int res = -1;

	if (flyingDirection == FLYING_DIRECTION_UP || flyingDirection == FLYING_DIRECTION_DOWN)
		res = MARIO_ANI_FIRE_SWITCHING_SCENE;
	else if (levelTransform)
	{
		if (nx > 0) res = MARIO_ANI_FIRE_IDLE_RIGHT;
		else res = MARIO_ANI_FIRE_IDLE_LEFT;
	}
	else if (sitting)
	{
		if (nx > 0) res = MARIO_ANI_FIRE_SITTING_RIGHT;
		else MARIO_ANI_FIRE_SITTING_LEFT;
	}
	else if (turning && !holdenKoopa && !jumping)
	{
		if (vx > 0) res = MARIO_ANI_FIRE_TURNING_LEFT;
		else res = MARIO_ANI_FIRE_TURNING_RIGHT;
	}
	else if (throwing)
	{
		if (nx > 0) res = MARIO_ANI_FIRE_RIGHT;
		else res = MARIO_ANI_FIRE_LEFT;
	}
	else
	{
		if (vx == 0)
		{
			if (keyFacing == MARIO_FACING_RIGHT)
			{
				if (jumping)
				{
					if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_JUMPING_RIGHT;
					else if (jumpMaxPower) res = MARIO_FIRE_JUMP_MAX_POWER_RIGHT;
					else res = MARIO_ANI_FIRE_JUMPING_RIGHT;
				}
				else if (kicking) res = MARIO_ANI_FIRE_KICKING_RIGHT;
				else
				{
					if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_IDLE_RIGHT;
					else res = MARIO_ANI_FIRE_IDLE_RIGHT;
				}
			}
			else
			{
				if (jumping)
				{
					if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_JUMPING_LEFT;
					else if (jumpMaxPower) res = MARIO_FIRE_JUMP_MAX_POWER_LEFT;
					else res = MARIO_ANI_FIRE_JUMPING_LEFT;
				}
				else if (kicking) res = MARIO_ANI_FIRE_KICKING_LEFT;
				else
				{
					if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_IDLE_LEFT;
					else res = MARIO_ANI_FIRE_IDLE_LEFT;
				}
			}
		}
		else if (keyFacing == MARIO_FACING_RIGHT)
		{
			if (jumping)
			{
				if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_JUMPING_RIGHT;
				else if (jumpMaxPower) res = MARIO_FIRE_JUMP_MAX_POWER_RIGHT;
				else res = MARIO_ANI_FIRE_JUMPING_RIGHT;

			}
			else if (kicking) res = MARIO_ANI_FIRE_KICKING_RIGHT;
			else if (vx * nx <= MARIO_WALKING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_WALKING_RIGHT;
				else res = MARIO_ANI_FIRE_WALKING_RIGHT;
			}
			else if (vx * nx <= MARIO_RUNNING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_RUNNING_RIGHT;
				else res = MARIO_ANI_FIRE_RUNNING_RIGHT;
			}
			else
			{
				if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_RUNNING_RIGHT;
				else res = MARIO_ANI_FIRE_RUNNING_FASTER_RIGHT;
			}
		}
		else if (keyFacing == MARIO_FACING_LEFT)
		{
			if (jumping)
			{
				if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_JUMPING_LEFT;
				else if (jumpMaxPower) res = MARIO_FIRE_JUMP_MAX_POWER_LEFT;
				else res = MARIO_ANI_FIRE_JUMPING_LEFT;

			}
			else if (kicking) res = MARIO_ANI_FIRE_KICKING_LEFT;
			else if (vx * nx <= MARIO_WALKING_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_WALKING_LEFT;
				else res = MARIO_ANI_FIRE_WALKING_LEFT;
			}
			else if (vx * nx <= MARIO_RUNNING_FAST_SPEED)
			{
				if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_RUNNING_LEFT;
				else res = MARIO_ANI_FIRE_RUNNING_LEFT;
			}
			else
			{
				if (holdenKoopa) res = MARIO_ANI_FIRE_HOLD_KOOPA_RUNNING_LEFT;
				else res = MARIO_ANI_FIRE_RUNNING_FASTER_LEFT;
			}
		}
	}
	return res;
}

int CMario::RenderLuigi()
{
	int res = -1;

	if (flyingDirection == FLYING_DIRECTION_UP || flyingDirection == FLYING_DIRECTION_DOWN)
		res = LUIGI_ANI_SWITCH_SCENE;
	else if (sitting)
	{
		if (nx > 0) res = LUIGI_ANI_SIT_RIGHT;
		else LUIGI_ANI_SIT_LEFT;
	}
	else if (turning)
	{
		if (vx > 0) res = LUIGI_ANI_TURN_LEFT;
		else res = LUIGI_ANI_TURN_RIGHT;
	}
	else
	{
		if (vx == 0)
		{
			if (keyFacing == MARIO_FACING_RIGHT)
			{
				if (jumping)
				{
					if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_JUMP_RIGHT;
					else res = LUIGI_ANI_JUMP_RIGHT;
				}
				else if (kicking) res = LUIGI_ANI_KICK_RIGHT;
				else
				{
					if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_IDLE_RIGHT;
					else res = LUIGI_ANI_IDLE_RIGHT;
				}
			}
			else
			{
				if (jumping)
				{
					if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_JUMP_LEFT;
					else res = LUIGI_ANI_JUMP_LEFT;
				}
				else if (kicking) res = LUIGI_ANI_KICK_LEFT;
				else
				{
					if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_IDLE_LEFT;
					else res = LUIGI_ANI_IDLE_LEFT;
				}
			}
		}
		else if (keyFacing == MARIO_FACING_RIGHT)
		{
			if (jumping)
			{
				if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_JUMP_RIGHT;
				else res = LUIGI_ANI_JUMP_RIGHT;

			}
			else if (kicking) res = LUIGI_ANI_KICK_RIGHT;
			else if (vx * nx <= MARIO_WALKING_SPEED)
			{
				if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_WALK_RIGHT;
				else res = LUIGI_ANI_WALK_RIGHT;
			}
			else if (vx * nx <= MARIO_RUNNING_SPEED)
			{
				if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_RUN_RIGHT;
				else res = LUIGI_ANI_RUN_RIGHT;
			}
			else
			{
				if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_RUN_RIGHT;
				else res = LUIGI_ANI_RUN_FAST_RIGHT;
			}
		}
		else if (keyFacing == MARIO_FACING_LEFT)
		{
			if (jumping)
			{
				if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_JUMP_LEFT;
				else res = LUIGI_ANI_JUMP_LEFT;

			}
			else if (kicking) res = LUIGI_ANI_KICK_LEFT;
			else if (vx * nx <= MARIO_WALKING_SPEED)
			{
				if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_WALK_LEFT;
				else res = LUIGI_ANI_WALK_LEFT;
			}
			else if (vx * nx <= MARIO_RUNNING_SPEED)
			{
				if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_RUN_LEFT;
				else res = LUIGI_ANI_RUN_LEFT;
			}
			else
			{
				if (holdenKoopa) res = LUIGI_ANI_HOLD_KOOPA_RUN_LEFT;
				else res = LUIGI_ANI_RUN_FAST_LEFT;
			}
		}
	}
	return res;
}

void CMario::SetSittingState(int state)
{
	int lastSittingState = sitting;

	if (state == MARIO_STATE_DIE)
	{
		sitting = 0;
		return;
	}

	if ((state == MARIO_STATE_IDLE || jumping == 1) && level != MARIO_LEVEL_SMALL)
		sitting = state;
	else if (isInIntro)
		sitting = state;
	else
		sitting = 0;

	if (!lastSittingState && sitting)
	{
		switch (level)
		{
		case MARIO_LEVEL_FIRE:
			y += MARIO_FIRE_BBOX_HEIGHT - MARIO_FIRE_SITTING_BBOX_HEIGHT;
			break;
		case MARIO_LEVEL_TAIL:
			y += MARIO_TAIL_BBOX_HEIGHT - MARIO_TAIL_SITTING_BBOX_HEIGHT;
			break;
		case MARIO_LEVEL_BIG:
			y += MARIO_BIG_BBOX_HEIGHT - MARIO_BIG_SITTING_BBOX_HEIGHT;
			break;
		}
	}
	else if (lastSittingState && !sitting)
	{
		switch (level)
		{
		case MARIO_LEVEL_FIRE:
			y -= MARIO_FIRE_BBOX_HEIGHT - MARIO_FIRE_SITTING_BBOX_HEIGHT;
			break;
		case MARIO_LEVEL_TAIL:
			y -= MARIO_TAIL_BBOX_HEIGHT - MARIO_TAIL_SITTING_BBOX_HEIGHT;
			break;
		case MARIO_LEVEL_BIG:
			y -= MARIO_BIG_BBOX_HEIGHT - MARIO_BIG_SITTING_BBOX_HEIGHT;
			break;
		}
	}
}

void CMario::SetJumpingUp(int jumpingUp)
{
	if (this->jumpingUp && !jumpingUp) floor = NULL;
	StartSpeedUp();
	this->jumpingUp = jumpingUp;
	if (jumpingUp) lastStandingHeight = this->y;
}

void CMario::FlyJump()
{
	if (level == MARIO_LEVEL_TAIL && jumping)
	{
		flyJump_start = (DWORD)GetTickCount64();
		flyJump = 1;
	}
}

void CMario::JumpMaxPower()
{
	if (level != MARIO_LEVEL_TAIL && powerLevel >= MAXIMUM_POWER_LEVEL && !jumpMaxPower)
	{
		jump_max_power_start = (DWORD)GetTickCount64();
		jumpMaxPower = MARIO_JUMP_MAX_POWER_UP;
	}
}

void CMario::SetThrowing()
{
	if (level != MARIO_LEVEL_FIRE) return;
	throwing = 1;
	throwing_start = (DWORD)GetTickCount64();
}

void CMario::ThrowFireball()
{
	if (level != MARIO_LEVEL_FIRE) return;
	
	currentFireball = -1;
	for (unsigned int i = 0; i < fireballs.size(); i++)
		if (fireballs[i]->GetState() == FIREBALL_STATE_ON_HOLD)
		{
			currentFireball = i;
			break;
		}

	if (currentFireball == -1) return;

	fireballs.at(currentFireball)->SetState(FIREBALL_STATE_ON_HOLD);
	if (nx > 0)
	{
		fireballs.at(currentFireball)->SetPosition(x + MARIO_FIRE_BBOX_WIDTH, y + MARIO_FIRE_BBOX_HEIGHT * 0.3f);
		fireballs.at(currentFireball)->SetDirection(FIREBALL_DIRECTION_RIGHT);
	}
	else
	{
		fireballs.at(currentFireball)->SetPosition(x - FIREBALL_BBOX_WIDTH, y + MARIO_FIRE_BBOX_HEIGHT * 0.3f);
		fireballs.at(currentFireball)->SetDirection(FIREBALL_DIRECTION_LEFT);
	}

	fireballs.at(currentFireball)->SetState(FIREBALL_STATE_FLY);
}

void CMario::releaseKoopa()
{
	holdenKoopa->SetHolden(0);
	
	int koopaState = holdenKoopa->GetState();
	if (koopaState == KOOPA_STATE_LYING_DOWN || koopaState == KOOPA_STATE_LYING_UP)
	{
		float l, t, r, b;
		GetBoundingBox(l, t, r, b);

		StartKicking();

		if (nx > 0)
		{
			holdenKoopa->SetPosition(r + 0.0001f, b - KOOPA_LYING_HEIGHT);
			if (holdenKoopa->GetState() == KOOPA_STATE_LYING_DOWN)
				holdenKoopa->SetState(KOOPA_STATE_ROLLING_DOWN_RIGHT);
			else
				holdenKoopa->SetState(KOOPA_STATE_ROLLING_UP_RIGHT);
		}
		else
		{
			holdenKoopa->SetPosition(l - KOOPA_LYING_WIDTH - 0.0001f, b - KOOPA_LYING_HEIGHT);
			if (holdenKoopa->GetState() == KOOPA_STATE_LYING_DOWN)
				holdenKoopa->SetState(KOOPA_STATE_ROLLING_DOWN_LEFT);
			else
				holdenKoopa->SetState(KOOPA_STATE_ROLLING_UP_LEFT);
		}
	}

	holdenKoopa = NULL;
}

void CMario::StartLevelTransform(int lastLevel, int newLevel)
{
	transform_lastLevel = lastLevel;
	transform_newLevel = newLevel;
	transformSteps = 0;
	levelTransform = 1;
	stepStart = (DWORD)GetTickCount64();
	SetLevel(transform_newLevel);
}

void CMario::LevelUp()
{
	if (level == MARIO_LEVEL_SMALL)
	{
		StartLevelTransform(level, MARIO_LEVEL_BIG);
	}
	else if (level == MARIO_LEVEL_BIG || level == MARIO_LEVEL_FIRE)
	{
		StartLevelTransform(level, MARIO_LEVEL_TAIL);
	}
}

void CMario::LevelDown()
{

	if (level == MARIO_LEVEL_FIRE)
	{
		StartUntouchable();
		StartLevelTransform(level, MARIO_LEVEL_BIG);
	}
	else if (level == MARIO_LEVEL_TAIL)
	{
		if (isInIntro)
		{
			StartLevelTransform(level, MARIO_LEVEL_SMALL);
		}
		else
		{
			StartUntouchable();
			StartLevelTransform(level, MARIO_LEVEL_BIG);
		}
	}
	else if (level == MARIO_LEVEL_BIG)
	{
		StartUntouchable();
		StartLevelTransform(level, MARIO_LEVEL_SMALL);
	}
	else {
		DebugOut(L"\nDIE");
		SetState(MARIO_STATE_DIE);
	}
}

void CMario::TurnIntoFire()
{
	if (level != MARIO_LEVEL_FIRE)
		StartLevelTransform(level, MARIO_LEVEL_FIRE);
}

void CMario::SetState(int state)
{
	lastState = GetState();
	lastRunning = running;
	int last_nx = nx;

	int stateCanBeChanged = 0;
	switch (state)
	{
	case MARIO_STATE_WALKING_RIGHT:
		background = 0;
		if (isInIntro) keyFacing = MARIO_FACING_RIGHT;

		if (((last_nx > 0 || vx == 0) && !turning) || jumping)
		{
			if (this->state != MARIO_STATE_WALKING_RIGHT) StartSpeedUp();

			running = 0;
			if (holdenKoopa && !allowHodingKoopa) releaseKoopa();
			vx = GetMarioExpectedSpeedX(MARIO_WALKING_SPEED);
			nx = 1;
			stateCanBeChanged = 1;
		}
		else if (!jumping)
		{
			if (!turning)
			{
				StartSpeedUp();
				turning = 1;
			}
			vx = GetMarioExpectedSpeedX(0.0f);
		}
		break;
	case MARIO_STATE_WALKING_LEFT:
		background = 0;
		if (isInIntro) keyFacing = MARIO_FACING_LEFT;

		if (((last_nx < 0 || vx == 0) && !turning) || jumping)
		{
			if (this->state != MARIO_STATE_WALKING_LEFT) StartSpeedUp();

			running = 0;
			if (holdenKoopa && !allowHodingKoopa) releaseKoopa();
			vx = GetMarioExpectedSpeedX(-MARIO_WALKING_SPEED);
			nx = -1;
			stateCanBeChanged = 1;
		}
		else if (!jumping)
		{
			if (!turning)
			{
				StartSpeedUp();
				turning = 1;
			}
			vx = GetMarioExpectedSpeedX(0.0f);
		}
		break;
	case MARIO_STATE_RUNNING_RIGHT:
		background = 0;
		if (isInIntro) keyFacing = MARIO_FACING_RIGHT;

		if (((last_nx > 0 || vx == 0) && !turning) || jumping)
		{
			if (this->state != MARIO_STATE_RUNNING_RIGHT) StartSpeedUp();

			running = 1;
			if (passedTheLevel)
				vx = MARIO_RUNNING_AFTERSCENE_SPEED;
			else
				vx = GetMarioExpectedSpeedX(MARIO_RUNNING_SPEED);
			nx = 1;
			stateCanBeChanged = 1;
		}
		else if (!jumping)
		{
			if (!turning)
			{
				StartSpeedUp();
				turning = 1;
			}
			vx = GetMarioExpectedSpeedX(0.0f);
		}
		break;
	case MARIO_STATE_RUNNING_LEFT:
		background = 0;
		if (isInIntro) keyFacing = MARIO_FACING_LEFT;

		if (((last_nx < 0 || vx == 0) && !turning) || jumping)
		{
			if (this->state != MARIO_STATE_RUNNING_LEFT) StartSpeedUp();

			running = 1;
			vx = GetMarioExpectedSpeedX(-MARIO_RUNNING_SPEED);
			nx = -1;
			stateCanBeChanged = 1;
		}
		else if (!jumping)
		{
			if (!turning)
			{
				StartSpeedUp();
				turning = 1;
			}
			vx = GetMarioExpectedSpeedX(0.0f);
		}

		break;
	case MARIO_STATE_RUNNING_FAST_RIGHT:
		background = 0;
		if ((last_nx > 0 && !turning) || jumping)
		{
			if (this->state != MARIO_STATE_RUNNING_FAST_RIGHT) StartSpeedUp();

			running = 1;
			vx = GetMarioExpectedSpeedX(MARIO_RUNNING_FAST_SPEED);
			nx = 1;
			stateCanBeChanged = 1;
		}
		else if (!jumping)
		{
			StartRunning();
			if (!turning)
			{
				StartSpeedUp();
				turning = 1;
			}
			vx = GetMarioExpectedSpeedX(0.0f);
		}
		break;
	case MARIO_STATE_RUNNING_FAST_LEFT:
		background = 0;
		if (last_nx < 0 && !turning)
		{
			if (this->state != MARIO_STATE_RUNNING_FAST_LEFT) StartSpeedUp();

			running = 1;
			vx = GetMarioExpectedSpeedX(-MARIO_RUNNING_FAST_SPEED);
			nx = -1;
			stateCanBeChanged = 1;
		}
		else if (!jumping)
		{
			StartRunning();
			if (!turning)
			{
				StartSpeedUp();
				turning = 1;
			}
			vx = GetMarioExpectedSpeedX(0.0f);
		}
		break;
	case MARIO_STATE_JUMPING:
		if (!turning)
		{
			background = 0;
			running = 0;
			if (powerLevel >= MAXIMUM_POWER_LEVEL)
			{
				if (level == MARIO_LEVEL_TAIL)
				{
					jumping = 1;
					FlyJump();
					if (fly != MARIO_FLYING_STATE_UP)
					{
						StartSpeedUp();
						fly = MARIO_FLYING_STATE_UP;
					}
				}
				else
				{
					JumpMaxPower();
				}
			}
			stateCanBeChanged = 1;
		}
		break; 
	case MARIO_STATE_IDLE:
		if (!jumping)
		{
			background = 0;
			if (this->state != MARIO_STATE_IDLE) StartSpeedUp();

			running = 0;
			if (holdenKoopa && !allowHodingKoopa) releaseKoopa();
			vx = GetMarioExpectedSpeedX(0.0f);
			stateCanBeChanged = 1;
		}
		break;
	case MARIO_STATE_IDLE_LEFT:
		background = 0;
		keyFacing = MARIO_FACING_LEFT;
		running = 0;
		SetSittingState(0);
		if (holdenKoopa && !allowHodingKoopa) releaseKoopa();
		vx = 0;
		nx = -1;
		stateCanBeChanged = 1;
		break;
	case MARIO_STATE_SIT_LEFT:
		background = 0;
		SetSittingState(1);
		vx = 0;
		nx = -1;
		break;
	case MARIO_STATE_SIT_RIGHT:
		background = 0;
		SetSittingState(1);
		vx = 0;
		nx = 1;
		break;
	case MARIO_STATE_DIE:
		running = 0;
		if (holdenKoopa && !allowHodingKoopa) releaseKoopa();
		vx = 0.0f;
		vy = -MARIO_DIE_DEFLECT_SPEED;
		StartDyingDelay();
		stateCanBeChanged = 1;
		break;
	case MARIO_STATE_JUMPING_OUT:
		background = 0;
		stateCanBeChanged = 1;
		break;
	case MARIO_STATE_LOOKING_UP:
		background = 0;
		stateCanBeChanged = 1;
		break;
	case MARIO_STATE_JUMPING_LEFT:
		background = 0;
		SetJumpingUp(1);
		SetState(MARIO_STATE_JUMPING);
		SetMovingDirection(MOVING_DIRECTION_LEFT);
		break;
	case MARIO_STATE_FLY_JUMP_LEFT:
		background = 0;
		SetMovingLeft(0);
		stateCanBeChanged = 1;
		break;
	case MARIO_STATE_IDLE_LEFT_HOLD_KOOPA:
		allowHodingKoopa = 1;
		SetState(MARIO_STATE_IDLE_LEFT);
		break;
	case MARIO_STATE_UNAVAILABLE:
		background = 1;
		stateCanBeChanged = 1;
		break;
	}

	if (jumping && vx != 0)
	{
		if (nx > 0)
			vx = GetMarioExpectedSpeedX(MARIO_JUMP_SPEED_X);
		else
			vx = GetMarioExpectedSpeedX(-MARIO_JUMP_SPEED_X);
	}

	if (last_nx != nx)
	{
		int leftMargin = 0, rightMargin = 0;

		GetMargins(leftMargin, rightMargin);

		if (last_nx > nx) x = x + leftMargin - rightMargin;
		else x = x - leftMargin + rightMargin;
	}

	if (stateCanBeChanged)
		CGameObject::SetState(state);
}

void CMario::SetLevel(int l)
{
	int lastLevel = level;

	float left, top, right, bottom;
	GetBoundingBox(left, top, right, bottom);
	float width = right - left, height = bottom - top;

	int leftMargin, rightMargin;
	GetMargins(leftMargin, rightMargin);

	level = l;

	int lv[] = { MARIO_LEVEL_SMALL, MARIO_LEVEL_BIG, MARIO_LEVEL_TAIL, MARIO_LEVEL_FIRE };

	int lv_height[] = { MARIO_SMALL_BBOX_HEIGHT, MARIO_BIG_BBOX_HEIGHT, MARIO_TAIL_BBOX_HEIGHT, MARIO_FIRE_BBOX_HEIGHT };
	int lv_width[] = { MARIO_SMALL_BBOX_WIDTH, MARIO_BIG_BBOX_WIDTH, MARIO_TAIL_BBOX_WIDTH, MARIO_FIRE_BBOX_WIDTH };

	int lv_sit_height[] = { MARIO_SMALL_BBOX_HEIGHT, MARIO_BIG_SITTING_BBOX_HEIGHT, MARIO_TAIL_SITTING_BBOX_HEIGHT, MARIO_FIRE_SITTING_BBOX_HEIGHT };
	int lv_sit_width[] = { MARIO_SMALL_BBOX_WIDTH, MARIO_BIG_SITTING_BBOX_WIDTH, MARIO_TAIL_SITTING_BBOX_WIDTH, MARIO_FIRE_SITTING_BBOX_WIDTH };

	int lv_width_marginLeft[] = { MARIO_SMALL_BBOX_MARGIN_LEFT, MARIO_BIG_BBOX_MARGIN_LEFT, MARIO_TAIL_BBOX_MARGIN_LEFT, MARIO_FIRE_BBOX_MARGIN_LEFT };
	int lv_width_marginRight[] = { MARIO_SMALL_BBOX_MARGIN_RIGHT, MARIO_BIG_BBOX_MARGIN_RIGHT, MARIO_TAIL_BBOX_MARGIN_RIGHT, MARIO_FIRE_BBOX_MARGIN_RIGHT };

	for (int j = 0; j < 4; j++)
	{
		if (lv[j] == level)
		{
			if (!sitting) y += height - lv_height[j];
			else y += height - lv_sit_height[j];

			if (nx > 0)
				x += (leftMargin - lv_width_marginLeft[j]);
			else
				x += (rightMargin - lv_width_marginRight[j]);
			return;
		}
	}
}

void CMario::GetMargins(int& leftMargin, int& rightMargin)
{
	switch (level)
	{
	case MARIO_LEVEL_SMALL:
		leftMargin = MARIO_SMALL_BBOX_MARGIN_LEFT;
		rightMargin = MARIO_SMALL_BBOX_MARGIN_RIGHT;
		break;
	case MARIO_LEVEL_BIG:
		leftMargin = MARIO_BIG_BBOX_MARGIN_LEFT;
		rightMargin = MARIO_BIG_BBOX_MARGIN_RIGHT;
		break;
	case MARIO_LEVEL_TAIL:
		leftMargin = MARIO_TAIL_BBOX_MARGIN_LEFT;
		rightMargin = MARIO_TAIL_BBOX_MARGIN_RIGHT;
		break;
	case MARIO_LEVEL_FIRE:
		leftMargin = MARIO_FIRE_BBOX_MARGIN_LEFT;
		rightMargin = MARIO_FIRE_BBOX_MARGIN_RIGHT;
		break;
	}
}

void CMario::GetBoundingBox(float &left, float &top, float &right, float &bottom)
{
	if (spinning && level == MARIO_LEVEL_TAIL)
	{
		float spinning_body = MARIO_TAIL_SPINNING_WIDTH - MARIO_TAIL_SPINNING_LENGTH;
		float body_face = MARIO_TAIL_FACING_SCREEN_WIDTH;

		if (spinningPhase == 1 || spinningPhase == 5)
		{
			if (nx > 0)
				left = x + MARIO_TAIL_SPINNING_LENGTH;
			else
				left = x;
		}
		else if (spinningPhase == 2 || spinningPhase == 4)
		{
			if (nx > 0)
				left = x;
			else
				left = x - spinning_body + body_face;
		}
		else if (spinningPhase == 3)
		{
			if (nx > 0)
				left = x;
			else
				left = x + MARIO_TAIL_SPINNING_LENGTH;
		}

		right = left + spinning_body;
		top = y;
		bottom = top + MARIO_TAIL_BBOX_HEIGHT;
		return;
	}

	left = x;
	top = y; 

	float leftMargin, rightMargin;

	if (level == MARIO_LEVEL_FIRE)
	{
		leftMargin = MARIO_FIRE_BBOX_MARGIN_LEFT;
		rightMargin = MARIO_FIRE_BBOX_MARGIN_RIGHT;

		if (nx < 0) swap(leftMargin, rightMargin);

		if (state == MARIO_STATE_IDLE)
			left = x + leftMargin;
		if (sitting)
		{
			right = x + MARIO_FIRE_SITTING_BBOX_WIDTH - rightMargin;
			bottom = y + MARIO_FIRE_SITTING_BBOX_HEIGHT;
		}
		else
		{
			right = x + MARIO_FIRE_BBOX_WIDTH - rightMargin;
			bottom = y + MARIO_FIRE_BBOX_HEIGHT;
		}
	}
	else if (level == MARIO_LEVEL_TAIL)
	{
		leftMargin = MARIO_TAIL_BBOX_MARGIN_LEFT;
		rightMargin = MARIO_TAIL_BBOX_MARGIN_RIGHT;

		if (keyFacing == MARIO_FACING_LEFT) swap(leftMargin, rightMargin);

		left = x + leftMargin;
		if (sitting)
		{
			right = x + MARIO_TAIL_SITTING_BBOX_WIDTH - rightMargin;
			bottom = y + MARIO_TAIL_SITTING_BBOX_HEIGHT;
		}
		else
		{
			right = x + MARIO_TAIL_BBOX_WIDTH - rightMargin;
			bottom = y + MARIO_TAIL_BBOX_HEIGHT;
		}
	}
	else if (level == MARIO_LEVEL_BIG || level == MARIO_LEVEL_LUIGI)
	{
		leftMargin = MARIO_BIG_BBOX_MARGIN_LEFT;
		rightMargin = MARIO_BIG_BBOX_MARGIN_RIGHT;

		if (keyFacing == MARIO_FACING_LEFT) swap(leftMargin, rightMargin);

		if (state == MARIO_STATE_IDLE || state == MARIO_STATE_JUMPING_OUT || state == MARIO_STATE_LOOKING_UP)
			left = x + leftMargin;
		if (sitting)
		{
			right = x + MARIO_BIG_SITTING_BBOX_WIDTH - rightMargin;
			bottom = y + MARIO_BIG_SITTING_BBOX_HEIGHT;
		}
		else
		{
			right = x + MARIO_BIG_BBOX_WIDTH - rightMargin;
			bottom = y + MARIO_BIG_BBOX_HEIGHT;
		}
	}
	else
	{
		leftMargin = MARIO_SMALL_BBOX_MARGIN_LEFT;
		rightMargin = MARIO_SMALL_BBOX_MARGIN_RIGHT;

		if (keyFacing == MARIO_FACING_LEFT) swap(leftMargin, rightMargin);

		if (state == MARIO_STATE_IDLE)
			left = x + leftMargin;
		right = x + MARIO_SMALL_BBOX_WIDTH - rightMargin;
		bottom = y + MARIO_SMALL_BBOX_HEIGHT;
	}
}

void CMario::SetSparkling()
{
	sparkle = 1;
	sparkle_start = (DWORD)GetTickCount64();
	GetSparklePosition(sparkle_x, sparkle_y);
}

void CMario::StartSpinning()
{
	if (spinning != 1 && level == MARIO_LEVEL_TAIL && sitting != 1)
	{
		spinning = 1;
		spinning_start = (DWORD)GetTickCount64();

		hittableTail = 1;
		if (nx > 0)
		{
			x -= MARIO_TAIL_SPINNING_LENGTH - MARIO_TAIL_NORMAL_LENGTH;
			SetTail(x, x + MARIO_TAIL_SPINNING_LENGTH);
		}
		else SetTail(x + MARIO_TAIL_SPINNING_WIDTH - MARIO_TAIL_SPINNING_LENGTH, x + MARIO_TAIL_SPINNING_WIDTH);

		spinningPhase = 1;
	}
}

int CMario::GetHittableTail()
{
	return this->hittableTail;
}

void CMario::SetTail(float start_x, float end_x)
{
	tail_start_x = start_x;
	tail_end_x = end_x;
	tail_start_y = y + MARIO_TAIL_HEAD_TO_TAIL;
	tail_end_y = tail_start_y + MARIO_TAIL_TAIL_WIDTH;
}

void CMario::ReducePowerLevel()
{
	if (powerLevel > 0)
	{
		if (GetTickCount64() - powerLevel_reduce_start > MARIO_POWER_LEVEL_REDUCING_TIME)
		{
			powerLevel--;
			powerLevel_reduce_start = (DWORD)GetTickCount64();
		}
	}
}

int CMario::OutOfCamera()
{
	float cx, cy;
	CCamera::GetInstance()->GetPosition(cx, cy);

	// Mario is out of camera
	if (y > cy + (float)CGame::GetInstance()->GetScreenHeight() || 
		x > cx + (float)CGame::GetInstance()->GetScreenWidth())
		return 1;
	return 0;
}

void CMario::StartSwitchingZone(int direction)
{
	float l, t, r, b;
	GetBoundingBox(l, t, r, b);
	float height = b - t;
	flyingSpeedY = MARIO_SWITCHING_SCENE_SPEED;
	disappear = 0;
	delayAfterMovingUp = 0;

	if (direction == MARIO_SWITCHING_ZONE_DIRECTION_UP)
	{
		maxFlyingY = minFlyingY = y - height;
		flyingDirection = FLYING_DIRECTION_UP;
	}
	else
	{
		minFlyingY = y;
		maxFlyingY = y + height;
		flyingDirection = FLYING_DIRECTION_DOWN;
	}
}

float CMario::GetMarioExpectedSpeedX(float limit_speed_x)
{
	if (isInIntro) return limit_speed_x;

	float result = 0.0f;
	DWORD t = (DWORD)GetTickCount64() - speed_up_start;	// Time range since Mario starting speed-up

	// Calculate acceleration sign to reach target speed
	float accelerationSign = 0;
	if (v0 > limit_speed_x)
		accelerationSign = MARIO_ACCELERATION_NEGATIVE;
	else
		accelerationSign = MARIO_ACCELERATION_POSITIVE;

	// Select appropriate acceleration value
	float acceleration = 0.0f;
	acceleration = MARIO_ACCELERATION_X;

	// Calculate expected speed (vx = v0 + a * t)
	result = v0 + accelerationSign * t * acceleration;

	// Limit result if it is exceeded expected velocity
	if ((accelerationSign == MARIO_ACCELERATION_POSITIVE && result > limit_speed_x) ||
		(accelerationSign == MARIO_ACCELERATION_NEGATIVE && result < limit_speed_x))
		result = limit_speed_x;

	return result;
}

void CMario::AddStreakScore(CGameObject* coObject)
{
	scoreStreak++;
	int addingScore = 0;
	vector<int> scoreSet = { MARIO_SCORE_100, MARIO_SCORE_200, MARIO_SCORE_400, MARIO_SCORE_800,
	MARIO_SCORE_1000, MARIO_SCORE_2000, MARIO_SCORE_4000, MARIO_SCORE_8000, MARIO_SCORE_1UP };

	if (scoreStreak <= scoreSet.size()) addingScore = scoreSet[scoreStreak - 1];
	else addingScore = scoreSet[scoreSet.size() - 1];
		
	AddScore(addingScore, coObject);
}

void CMario::AddScore(int score, CGameObject* coObject)
{
	if (score != MARIO_SCORE_1UP) {
		CHUD::GetInstance()->AddScore(score);
	}
	else {
		CHUD::GetInstance()->AddLives(1);
	}

	CScores::GetInstance()->CreateNewScoreObject(score, coObject);
}

/*
	Reset Mario status to the beginning state of a scene
*/
void CMario::Reset()
{
	SetState(MARIO_STATE_IDLE);
	SetLevel(MARIO_LEVEL_BIG);
	SetPosition(start_x, start_y);
	SetSpeed(0, 0);
}

