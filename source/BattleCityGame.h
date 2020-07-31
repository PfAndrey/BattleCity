#ifndef BATTLECITYGAME_H
#define BATTLECITYGAME_H

#include <vector>
#include <array>
#include "assert.h"
#include <memory>
#include <iostream>
#include "GameEngine.h"

#include <future>
#include <thread> 

enum ETiles { empty, brick, armor, wood, border, lake };

class CBattleCityGameScene;
class CBattleCityMenuScene;
class CMap;

namespace BattleCityConsts
{
	const Vector MAP_SIZE{ 28, 28 };
	const int ETiles_SIZE = 25;
	const Vector ENEMY_SPAWN_TILES[3] = { { 1,1 },{ 13,1 } ,{ 25,1 } };
	const Vector PLAYER_SPAWN_TILE = { 10, 25 };
	const Vector EAGLE_TILE = { 13, 25 };
	const Vector EAGLE_SIZE = { 50, 50 };
	const int BONUS_TANKS_ST = 6;
	const int ENEMY_SPAWN_TIME = 3000; //ms
	const int BONUS_TIME_ALIVE = 14000; //ms
	const int TIME_OF_FREEZING = 7000; //ms
	const int TIME_OF_HELMET = 10000; //ms
	const int TIME_OF_SHOVEL = 10000; //ms
}

class CBattleCityGame : public CGame
{
private:
	CBattleCityGame();
	CBattleCityGameScene* m_game_scene;
	CBattleCityMenuScene* m_menu_scene;
	static CBattleCityGame* s_instance;
 
public:
	~CBattleCityGame();
	static CBattleCityGame* CBattleCityGame::instance();
	void init() override;
};


class CTank;

class CBullet : public CGameObject
{
public:
	CBullet(const Vector& pos, const Vector& speed_vector,  CTank* source, bool is_armor_piercing = false);
	void update(int delta_time) override;
	void draw(sf::RenderWindow* render_window) override;
	Rect getBounds() const override;
	void detonate(bool silent = false);
	CGameObject* source() const;
	bool isDetonated() const;
	bool isArmorPiercing() const;
private:
	Vector m_speed_vector;
	Animator m_animator;
	bool m_detonate;
	int m_death_timer = 1000;
	CTank* m_source;
	bool m_is_armor_piercing;

};

class CTank : public CGameObject
{
public:
	CTank(CMap* map);
	void update(int delta_time) override;
	void draw(sf::RenderWindow* window) override;
	void setBodyColor(const sf::Color& color);
	enum class EState { borning, normal, detonate };
	void detonate();
	virtual void damage();
	bool isAlive() const;
	void setState(EState state);
	EState getState() const;
	void turnOnShield();
	void turnOffShield();
	bool isShielding() const;
	void onBulletDetonated();
	void setSpeed(float value);
    float getSpeed() const;
	virtual void stop();
protected:
	void setBodySprite(const Rect& sprite_rect);
	int bulletsInMoving() const;
	void fire(bool armored = false);
	bool isOvercharged() const;
	float m_speed = 0;
	CMap* m_map;
	int m_time;
	Animator m_animator;
	float m_bullet_speed = 0.5;
	int m_firing_rate = 300;
	int last_fire_time;
	float m_tank_max_speed = 0.1f;
	int m_health = 1;

private:
	int m_bullets_in_moving = 0;
	EState  m_state;
	CSpriteSheet* m_shield_sh;
	bool m_space_pressed;
	bool m_shielding;
 
};

class CTankPlayer : public CTank
{
public:
	CTankPlayer(CMap* map);
	void update(int delta_time) override;
	void setRank(int rank);
	int getRank() const;
	void promote();
	void spawn(const Vector& position, const Vector& direction);
private:
	int m_rank = 0;
	bool m_space_pressed = false;
};

class CEnemyTank : public CTank
{
public:
	enum Type { basic = 0, fast, power, armor };
	CEnemyTank(CMap* map, CTankPlayer* player, Type type);
	void setFlashed(bool value);
	bool isFlashing() const;
	static void setFreezed(bool value = true);
	static bool isFreezed();
	Type type() const;
	virtual void damage() override;
	virtual void stop() override;
private:
	WaypointSystem* m_waypoint_system = NULL;
	void update(int delta_time) override;
	bool moveToPoint(const Vector& target_point);
	bool moveInRandomDirection();
	int m_timer = 0;
	CTankPlayer* m_player;
	int m_remove_timer;
	const int m_period_time = 3000;
	int m_last_move_update  = 0;
	bool m_flashed = false;
	static bool m_freezed;
	Type m_type;
};

class CEagle : public CGameObject
{
public:
	CEagle();
	void detonate();
	bool isDetonated() const;
	void update(int delta_time) override;
	void draw(sf::RenderWindow* render_window) override;
	void setNormalState();
private:
	CSpriteSheet* m_explosion_sh;
	CSpriteSheet* m_body_sh;
	bool m_detonated = false;
};

class CMap;
class CBonus;
class LifeBar;
class CCurtains;

class CBattleCityGameScene : public CGameObject
{
  public:
	  CBattleCityGameScene();
	  void update(int delta_time);
	  void reset();
	  void addLifeToPlayerTank();
	  void removeLifeFromPlayerTank();
	  void blowupAllTanks();
	  void hideHUD();
	  void showHUD();
	
private:
	  void loadStage(int stage_index);
  	  CEnemyTank* spawnEnemyTank();
	  void spawnPlayerTank();
	  CBonus* getRandomBonus();
	  void addScore(int score);
	  CFlowText* m_float_text;
	  int m_score ;
	  int m_player_tanks_lifes;
	  int m_stage_index;
	  CMap* m_walls;
	  CEagle* m_eagle;
	  CTankPlayer* m_player;
	  int m_enemy_spawn_counter;
	  int m_enemy_crash_counter;
	  int m_enemy_spawn_timer;
	  const int m_tanks_on_level = 20;
	  bool m_freezed_mode = false;
	  LifeBar* m_enemy_tanks_bar = NULL;
	  CLabel* m_score_label = NULL;
	  CLabel* m_lifes_label = NULL;
	  CLabel* m_stage_label = NULL;
	  CLabel* m_game_over_label = NULL;
	  CCurtains* m_curtains = NULL;
	  CLabel* m_bar_background = NULL;
	  int m_need_next_level_state;
	  int m_need_game_over_state;
	  int m_game_over_timer = 0;
	  int m_next_level_timer = 0;
	  int m_dy = 0;
};

class CBattleCityMenuScene : public CGameObject
{
public:
	CBattleCityMenuScene();
	void update(int delta_time);
	void reset();
private:
	CLabel* m_cursor_label;
	CLabel* m_logo_label;
	CLabel* m_one_player_label;
	CLabel* m_two_player_label;
	CLabel* m_constructor_label;
	CLabel* m_about_label;
	int m_cursor_pos;
	int m_dy;
};
 
class CMap : public CGameObject
{
private:
	int m_water_index = 0;
	HPA_Finder<ETiles> m_HPA_finder;
	const int tile_size = 25;
	TileMap<ETiles> m_map;
	CSpriteSheet m_sprite_sheet;
	sf::RectangleShape m_shape;
	sf::Sprite m_eagle_sprite;
	int m_timer = 0;
	bool hpa_blocked = false;
public:
	CMap(int width, int height);
	void draw(sf::RenderWindow* render_window) override;
	void postDraw(sf::RenderWindow* render_window) override;
	void update(int delta_time) override;
	TileMap<ETiles>* getMap();
	Vector toPixelCoordinates(const Vector& point);
	Rect toPixelCoordinates(const Rect& rect);
	std::vector<Vector> toPixelCoordinates(const std::vector<Vector>& points);
    Vector toMapCoordinates(const Vector& point,bool rounded = false);
	Vector alignToTiles(const Vector& pos)
	{
	    return round(pos / tile_size)*tile_size;
	}
	
	bool isCollide(Rect& rect, const std::vector<ETiles>& allowed_cell_types);
	HPA_Finder<ETiles>& HPA_Finder()
	{
		return m_HPA_finder;
	}
};

class CBonus : public CGameObject
{
 public:
	 CBonus();
	 void postDraw(sf::RenderWindow* render_window);
	 void update(int delta_time);
	 void pickup();
	 bool isPickuping() const;
	 virtual void reset();
 protected:
	 void setSprite(const sf::Sprite& sprite);
	 int getTime() const;
	 void resetTime();
 private:
	 bool m_pickuped = false;
	 float m_timer = 0;
	 const int size = 50;
	 sf::Sprite m_sprite;
};


class CGrenede : public CBonus
{
public:
	CGrenede();
	void update(int delta_time) override;
	void detonate();

};

class CFreezer : public CBonus
{
public:
	CFreezer();
	void update(int delta_time) override;
	virtual void reset();
private:
	int m_step = 0;
};

class CHelmet : public CBonus
{
public:
	CHelmet();
	void update(int delta_time) override;
	virtual void reset();
private:
	int m_step = 0;
};

class CShovel : public CBonus
{
private:
	int m_step = 0;
	std::vector<Vector> m_cells;
	CMap* m_map = nullptr;
	void fence(const ETiles& tile);
	virtual void onActivated() override;
	virtual void reset();
public:
	CShovel();

	void update(int delta_time) override;

};

class LifeBar : public CGameObject
{
public:
	LifeBar(const sf::Sprite& life_sprite, int cols, int rows);
	void setBackgroundColor(const sf::Color& color);
	void setValue(int value);
	void decrease()
	{
		m_value--;
	}
	void draw(sf::RenderWindow* render_window) override;

  private:
	  int m_value;
	  int m_rows, m_cols;
	  sf::Color m_background_color;
	sf::Sprite m_life_sprite;
};

class CStar : public CBonus
{
public:
	CStar();
	void update(int delta_time) override;
};

class CLife : public CBonus
{
public:
	CLife();
	void update(int delta_time) override;
};

class CCurtains : public CGameObject
{
 public:
	 CCurtains(const Rect& rect, const sf::Font& font)
	 {
		 setBounds(rect);
		 m_up_curtain.setPosition({0,0});
		 m_up_curtain.setSize({ rect.width(),rect.height() / 2 });

		  m_down_curtain.setPosition({0,rect.height()/2});
		  m_down_curtain.setSize({ rect.width(),rect.height() / 2 });

		 m_up_curtain.setFillColor(sf::Color(180, 180, 180));
		 m_down_curtain.setFillColor(sf::Color(180,180,180));
		 m_label = new CLabel();
		 m_label->setFontName(font);
		 m_label->setPosition(rect.center());
		 
		 m_shadow.setPosition({ 0,0 });
		 m_shadow.setSize(rect.size());
		 m_shadow.setFillColor(sf::Color::Black);

		 
		 m_state = 3;
	 }
	 ~CCurtains()
	 {
		 delete m_label;
	 }
	 void play(const std::string& text, bool shadowed = false)
	 {
		 m_shadowed = shadowed;
		 m_timer = 0;
		 m_state = 0;
		 m_label->setString(text);
	 }

	 void postDraw(sf::RenderWindow* render_window)
	 {
		 if (m_shadowed)
			 if (m_state == 0)
				 render_window->draw(m_shadow);

		 render_window->draw(m_up_curtain);
		 render_window->draw(m_down_curtain);
		 if (m_timer > 200 && m_timer < 800)
		  m_label->draw(render_window);
	 }


	 void update(int delta_time)
	 {
		 CGameObject::update(delta_time);

		 auto size = getBounds().size();

		 
		 //States
		 if (m_state == 0)
			 m_h+=0.3* delta_time;
		 else if (m_state == 1)
			 m_timer += delta_time;
		 else if (m_state == 2)
			 m_h -= 0.3* delta_time;
		 
		 //Trasitions
		 if (m_state == 0 && m_h > (size.y /2)) m_state = 1;
		 if (m_state == 1 && m_timer > 1000) { m_timer = 0;  m_state = 2; }
		 if (m_state == 2 && m_h < 0 ) m_state = 3;



		 m_up_curtain.setPosition(sf::Vector2f(0,  - size.y/2  +  m_h ));
		 m_down_curtain.setPosition( 0, size.y - m_h );

	 }

 private:
	 float m_h = 0;
	 int m_timer;
	 int m_state = 0;
	 bool m_shadowed = false;
	 sf::RectangleShape m_up_curtain;
	 sf::RectangleShape m_down_curtain;
	 sf::RectangleShape m_shadow;
	 CLabel* m_label;
};

class CHPAVisualiser : public CGameObject
{
private:
	CMap* m_map;
	HPA_Finder<ETiles> m_HPA_Finder;
	std::vector<Vector> m_path;

public:
	CHPAVisualiser(CMap* map);
	void refresh();
	void draw(sf::RenderWindow* render_window) override;
};



#endif