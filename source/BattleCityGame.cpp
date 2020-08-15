
#include "BattleCityGame.h"
#include "Pickups.h"

const AllowedCellPredicate<ETiles> ALLOWED_CELL_PREDICATE = [](const ETiles& type)
{
	if (type == ETiles::empty || type == ETiles::wood)
	{
		return true;
	}
	return false;
};

//--------------------------------------------------------------------------------------

CBattleCityGame* CBattleCityGame::s_instance = NULL;

CBattleCityGame* CBattleCityGame::instance()
{
	if (s_instance == NULL)
		s_instance = new CBattleCityGame();
	return s_instance;
}
 
CBattleCityGame::CBattleCityGame() : CGame("Battle City", { 825, 700 })
{
	//Load textures
	const std::string textures_dir = "res/Textures/";
	for (auto texture : { "battle_city_sheet", "explosion_sheet", "battle_city_logo" })
		textureManager().loadFromFile(texture, textures_dir + texture + ".png");

	//Load fonts
	const std::string fonts_dir = "res/Fonts/";
	for (auto font : { "menu_font", "main_font", "score_font", "some_font" })
		fontManager().loadFromFile(font, fonts_dir + font + ".ttf");

	//Load sounds
	const std::string sounds_dir = "res/Sounds/";
	for (auto sound : { "stage_start", "1-up", "armor-push", "block-broken", "bonus-picked",
		"bonus-spawned", "damage", "enemy-boom", "player-boom", "fire", "click", "pause", "unpause" } )
		soundManager().loadFromFile(sound, sounds_dir + sound + ".ogg");
 
	//Configure input
	std::vector<std::pair<std::string, std::vector<std::string>>> inputs =
	{
		{ "Fire",{ "Space",  "[1]" } },
		{ "Horizontal+",{ "Right" } },
		{ "Horizontal-",{ "Left" } },
		{ "Vertical-",{ "Up" } },
		{ "Vertical+",{ "Down" } }
	};

	for (auto input : inputs)
	{
		inputManager().setupButton(input.first, input.second);
	}
}

CBattleCityGame::~CBattleCityGame()
{

}

bool need_redraw = true;

void CBattleCityGame::init()
{
	m_game_scene = new CBattleCityGameScene();
	m_menu_scene = new CBattleCityMenuScene();
	getRootObject()->addObject(m_game_scene);
	getRootObject()->addObject(m_menu_scene);
	m_game_scene->turnOff();
}
//----------------------------------------------------------------------------------------------

CBullet::CBullet(const Vector& pos, const Vector& speed_vector,  CTank* source, bool is_armor_piercing):
	m_is_armor_piercing(is_armor_piercing),
	m_source(source),
	m_speed_vector(speed_vector),
	m_detonate(false)
{
	setName("Bullet");
	m_animator.create("fly", *CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { 130, 160, 14, 8 });
	m_animator.create("explosion", *CBattleCityGame::instance()->textureManager().get("explosion_sheet"), { 0, 184 }, { 64, 64 }, 4, 3, 0.01);
	m_animator.get("explosion")->setOrigin({ 32,32 });
	m_animator.get("fly")->setRotation(-speed_vector.angle() + 90);
	setDirection(speed_vector.normalized());
	setPosition(pos);
}

CGameObject* CBullet::source() const
{
	return m_source;
}

void CBullet::update(int delta_time)
{
	CGameObject::update(delta_time);
	
	if (m_detonate)
	{
		m_death_timer -= delta_time;
		if (m_death_timer < 0)
		{
			getParent()->removeObject(this);
		}
	}

	move(delta_time*m_speed_vector);

	m_animator.update(delta_time);
}

Rect CBullet::getBounds() const  
{
	return Rect(getPosition(),   Vector(14, 8));
}

void CBullet::detonate(bool silent)
{
	if (!m_detonate)
	{
		if (silent)
			hide();
		else
			m_animator.play("explosion");
		m_detonate = true;;
		m_speed_vector = Vector::zero;
	}
	m_source->onBulletDetonated();
}

bool CBullet::isDetonated() const
{
	return m_detonate;
}

bool CBullet::isArmorPiercing() const
{
	return 	m_is_armor_piercing;
}

void CBullet::draw(sf::RenderWindow* render_window)
{
	m_animator.draw(render_window);
	m_animator.setPosition(getPosition());
}

//----------------------------------------------------------------------------------------------

CTank::CTank(CMap* map):
	m_tank_max_speed(0.1f),
	m_map(map),
	m_last_fire_time(0)
{
	setName("Tank");
	auto& text_manager = CBattleCityGame::instance()->textureManager();

	m_animator.create("borning", *text_manager.get("battle_city_sheet"), { 0, 100 }, { 50, 50 }, 4, 1, 0.01, AnimType::forward_backward_cycle);
	m_animator.create("explosion", *text_manager.get("explosion_sheet"), { 0, 0 }, { 92, 92 }, 5, 2, 0.01, AnimType::forward);
	m_animator.get("explosion")->setOrigin({ -12,10 });

	m_shield_sh = new CSpriteSheet();
	m_shield_sh->load(*text_manager.get("battle_city_sheet"), Vector(0, 150), Vector(50, 50), 2, 1);
	m_shield_sh->setOrigin({ 0,0 });
	m_shield_sh->setAnimType(AnimType::forward_cycle);
	m_shield_sh->setSpeed(0.01);

	turnOffShield();
	setState(EState::borning);
	setSize({ 50,50 });
}

void CTank::turnOnShield(int time)
{
	m_shield_sh->turnOn();
	m_shielding = time;
}

void CTank::turnOffShield()
{
	m_shield_sh->turnOff();
	m_shielding = 0;
}

bool CTank::isShielding() const
{
	return m_shielding >=0 ;
}

void CTank::setState(EState state)
{
	m_state = state;
 
	switch (state)
	{
		case(EState::borning):
		{
			m_animator.play("borning");
			break;
		}
		case(EState::detonate):
		{
			m_animator.play("explosion");
			stop();
			break;
		}
		case(EState::normal):
		{
			break;
		}
	}

	m_time = 0;
}

void CTank::update(int delta_time)
{
	CGameObject::update(delta_time);

	switch (m_state)
	{
		case(EState::borning):
		{
			if (m_time > 1000)
			{
				setState(EState::normal);
			}
			break;
		}
		case(EState::detonate):
		{
			break;
		}
		case(EState::normal):
		{
			m_last_fire_time += delta_time;
			if (isShielding())
			{
				m_shield_sh->update(delta_time);
				m_shielding -= delta_time;
			}
			break;
		}
	}

	m_time += delta_time;
	m_animator.update(delta_time);
}

CTank::EState CTank::getState() const
{
	return m_state;
}

void CTank::fire(bool armored)
{
	m_last_fire_time = 0;
	getParent()->addObject(new CBullet(getBounds().center() + getDirection() * 25, getDirection()*m_bullet_speed, this, armored));
	m_bullets_in_moving++;
}

void CTank::onBulletDetonated()
{
	m_bullets_in_moving--;
}

int CTank::bulletsInMoving() const
{
	return m_bullets_in_moving;
}

void CTank::setBodyColor(const sf::Color& color)
{
	m_animator.setColor(color);
}

void CTank::detonate()
{
	setState(CTank::EState::detonate);
}

bool CTank::isAlive() const
{
	return m_state == CTank::EState::normal;
}

bool CTank::isDetonated() const
{
	return m_state == CTank::EState::detonate;
}

void CTank::damage()
{
	if (!isShielding() && isAlive())
	{
		m_health--;
		if (m_health <= 0)
		{
			detonate();
		}
	}
}

bool CTank::isOvercharged() const
{
	return m_last_fire_time > m_firing_rate;
}

void CTank::draw(sf::RenderWindow* window)  
{
	m_animator.setPosition(getPosition());
	m_animator.draw(window);
 
	if (isShielding() && isAlive())
	{
		m_shield_sh->setPosition(getPosition());
		m_shield_sh->draw(window);
	}
	
	CGameObject::draw(window);
}

void CTank::setSpeed(float value)
{
	m_speed = value;
}

float CTank::getSpeed() const
{
	return m_speed;
}

void CTank::setBodySprite(const Rect& sprite_rect)
{

}

void CTank::stop()
{
	setSpeed(0);
}

//----------------------------------------------------------------------------------------------

CTankPlayer::CTankPlayer(CMap* map): CTank(map)
{
	setName("PlayerTank");
	setDirection(Vector::up);
	
	auto texture = CBattleCityGame::instance()->textureManager().get("battle_city_sheet");

	for (int i = 0; i < 4; ++i)
	{
		std::string rank = "_" + toString(i);
		m_animator.create("right"+rank, *texture, Rect(i *50,50,50,50));
		m_animator.create("left"+rank, *texture, Rect(50 + i * 50,50,-50,50 ));
		m_animator.create("up"+rank, *texture, Rect(i * 50,50,50,50 ));
		m_animator.create("down"+rank, *texture, Rect(i * 50,50,50,50 ));
		m_animator.get("up"+rank)->setRotation(270);
		m_animator.get("down"+rank)->setRotation(90);
	}
 
	turnOnShield(4000);
	setBodyColor(sf::Color(255,255,102));
	setRank(0);
}

void CTankPlayer::update(int delta_time)
{
	CTank::update(delta_time);

	switch (getState())
	{
		case(EState::normal):
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) setRank(0);
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) setRank(1);
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) setRank(2);
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) setRank(3);

			Vector input_direction = CBattleCityGame::instance()->inputManager().getXYAxis();
			
			if (input_direction.x && input_direction.y)
			{
				input_direction.y = 0; //can move only for one axis 
			}

			Vector old_direction = getDirection();

			if (input_direction != Vector::zero)
			{
				setDirection(input_direction);
			}

			if (old_direction != getDirection()) //align on map
			{
				setPosition(m_map->alignToTiles(getPosition()));

				updateSprite();
			}

			if (input_direction != Vector::zero)
			{
				setSpeed(m_tank_max_speed);
				move(getDirection() * delta_time * getSpeed());
			}

			if (CBattleCityGame::instance()->inputManager().isButtonDown("Fire"))
			{
				if (m_rank <= 1)
				{
					if (isOvercharged() && !bulletsInMoving())
					{
						fire();
					}
				}
				else
				{
					if (isOvercharged() && bulletsInMoving() < 2)
					{
						fire(getRank() >= 3 ? true : false);
						if (bulletsInMoving() == 2) //two bullets
						{
							m_last_fire_time -= 200;  //with 200 msec interval
						}
					}

				}
			}
			
			break;
		}
	}
}

void CTankPlayer::updateSprite()
{
	auto direction = getDirection();
	if (direction == Vector::right)  m_animator.play("right_" + toString(m_rank));
	else if (direction == Vector::left)  m_animator.play("left_" + toString(m_rank));
	else if (direction == Vector::up)  m_animator.play("up_" + toString(m_rank));
	else if (direction == Vector::down)  m_animator.play("down_" + toString(m_rank));
}

void CTankPlayer::setRank(int rank)
{
	assert(rank >= 0 && rank < 4);
	m_rank = rank;

	if (m_rank == 0)
	{
		m_bullet_speed = BattleCityConsts::BASIC_BULLET_SPEED;
	}
	else
	{
		m_bullet_speed = BattleCityConsts::FAST_BULLET_SPEED;
	}

	updateSprite();
}

int CTankPlayer::getRank() const
{
	return m_rank;
} 

void CTankPlayer::promote()
{
	int rank = m_rank + 1;
	if (rank >= 3)
	{
		m_health = 2;
		rank = 3;
	}
	setRank(rank);
}

void CTankPlayer::spawn(const Vector& position, const Vector& direction)
{
	setRank(0);
	m_health = 1;
	setState(CTank::EState::borning);
	setPosition(position);
	setDirection(direction);
	turnOnShield(4000);
}

void CTankPlayer::fire(bool armored)
{
	CTank::fire(armored);
	CBattleCityGame::instance()->playSound("fire");
}
//------------------------------------------------------------------------------------------------------

CEnemyTank::CEnemyTank(CMap* map, CTankPlayer* player, Type type) 
	: CTank(map),
	m_type(type),
	m_player(player),
	m_remove_timer(0)
{
	setName("EnemyTank");
	addObject(m_waypoint_system = new WaypointSystem());
	setDirection(Vector::up);
	setSpeed(0);
 
    int index = (int)m_type;
	auto texture = CBattleCityGame::instance()->textureManager().get("battle_city_sheet");
	m_animator.create("right", *texture, Rect( 50*index,200,50,50 ));
	m_animator.create("left", *texture, Rect(50 + 50 * index,200,-50,50 ));
	m_animator.create("up", *texture, Rect(50 * index,200,50,50 ));
	m_animator.create("down", *texture, Rect(50 * index,200,50,50 ));
	m_animator.get("up")->setRotation(270);
	m_animator.get("down")->setRotation(90);
 
	switch (m_type)
	{
		case(Type::basic):
		{
			m_bullet_speed = BattleCityConsts::BASIC_BULLET_SPEED;
			m_health = 1;
			break;
		}
		case(Type::armor):
		{
			m_bullet_speed = BattleCityConsts::FAST_BULLET_SPEED;
			m_health = 2;
			break;
		}
		case(Type::fast):
		{
			m_bullet_speed = BattleCityConsts::BASIC_BULLET_SPEED;
			m_health = 1;
			break;
		}
		case(Type::power):
		{
			m_bullet_speed = BattleCityConsts::BASIC_BULLET_SPEED;
			m_health = 4;
			break;
		}
	}
}

bool CEnemyTank::moveToPoint(const Vector& target_point)
{
	setPosition(m_map->alignToTiles(getPosition()));

	Vector own_cell = m_map->toMapCoordinates(getPosition(), true);

	 if ( (target_point - own_cell).length() <= 1)
		 return false;

	if (m_map->getMap()->getCell(target_point) == ETiles::empty)
	{
		//auto path = m_map->getMap()->findPath(own_cell, target_point, [](const ETiles& tile) {return tile == ETiles::empty; }, 2);
		auto path = m_map->HPA_Finder().search(own_cell, target_point);

		if (path.size() > 1)
		{
			setSpeed(m_tank_max_speed);
			m_waypoint_system->addPath(m_map->toPixelCoordinates(path),getSpeed(),true);  
			return true;
		}
	}

	return false;
}

bool CEnemyTank::moveInRandomDirection()
{
	m_waypoint_system->stop();

 	setPosition(m_map->alignToTiles(getPosition()));

	Vector tank_cell = m_map->toMapCoordinates(getPosition(), true);
	Vector new_direction = ((rand() % 2) ? rotateClockwise(getDirection()) : rotateAnticlockwise(getDirection()));
 
	setDirection(new_direction);
	Vector target = tank_cell + new_direction * 100;

	if (target != tank_cell)
	{
		setSpeed(m_tank_max_speed);
		std::vector<Vector> path = { tank_cell, target };
		m_waypoint_system->addPath(m_map->toPixelCoordinates(path), getSpeed(),true);  
		return true;
	}

	return false;
}

void CEnemyTank::update(int delta_time)
{
	CTank::update(delta_time);

	if (isFreezed())
		return;

	if (getState() == EState::detonate)
	{
		m_remove_timer += delta_time;
		if (m_remove_timer > 1000)
		{
			getParent()->removeObject(this);
		}
	}
	else if (getState() == EState::normal)
	{
		m_last_move_update += delta_time;
		m_timer += delta_time;

		if (m_last_move_update > 1500 || (getSpeed() == 0 && m_last_move_update > 250))
		{
			m_last_move_update = 0;

			//Enemy Tank AI -------------------------------------
			bool path_finded = true;
			if (m_timer < m_period_time) // Random move of the 1/3 time
			{
				path_finded = moveInRandomDirection();
			}
			else if (m_timer < 2 * m_period_time) // attack player tank
			{
				path_finded = moveToPoint(m_map->toMapCoordinates(m_player->getPosition(), true));
			}
			else if (m_timer < 3 * m_period_time) // attack eagle
			{
				path_finded = moveToPoint(BattleCityConsts::EAGLE_TILE);
			}
			else
			{
				m_timer = 0;
			}
			//------------------------------------------------------
		}

		if (m_flashed)
		{
			float color = 200 + 50 * cos(m_timer / 150);
			setBodyColor(sf::Color(color, color, color));
		}

		int max_bulles = (m_type == Type::armor) ? 2 : 1;
		if (isOvercharged() && bulletsInMoving() < max_bulles && std::rand() % 32 == 0)
		{
			fire();
		}

		auto direction = getDirection();
		if (direction == Vector::right)  m_animator.play("right");
		else if (direction == Vector::left)  m_animator.play("left");
		else if (direction == Vector::up)  m_animator.play("up");
		else if (direction == Vector::down)  m_animator.play("down");
	}
}

void CEnemyTank::setFlashed(bool value)
{
	m_flashed = value;
	if (!m_flashed)
	  m_animator.setColor(sf::Color::White);
}

bool CEnemyTank::isFlashing() const
{
	return m_flashed;
}

void CEnemyTank::setFreezed(bool value)
{
	m_freezed = value;
}

bool CEnemyTank::isFreezed()
{
	return m_freezed;
}

CEnemyTank::Type CEnemyTank::type() const
{
	return m_type;
}

void CEnemyTank::damage() 
{
	CTank::damage();
	if (m_type == Type::armor && !isShielding())
	{
		sf::Uint8 color = 255 - 30 * (4 - m_health);
		setBodyColor({ color ,color ,color });
	}
}

void CEnemyTank::stop()
{
	CTank::stop();
	m_waypoint_system->stop();
}

bool CEnemyTank::m_freezed = false;

//-------------------------------------------------------------------------------------------------------

CEagle::CEagle()
{
	m_body_sh = new CSpriteSheet();
	m_body_sh->load(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { { 100,0,50,50 },{ 150,0,50,50 } });
	addObject(m_body_sh);
	m_explosion_sh = new CSpriteSheet();
	m_explosion_sh->load(*CBattleCityGame::instance()->textureManager().get("explosion_sheet"), Vector(0, 0), Vector(92, 92), 5, 2);
	m_explosion_sh->setOrigin({ -12,-10 });
	m_explosion_sh->setAnimType(AnimType::forward);
	m_explosion_sh->turnOff();
	m_explosion_sh->setSpeed(0.005);
	addObject(m_explosion_sh);
	setSize(BattleCityConsts::EAGLE_SIZE);
}

void CEagle::detonate()
{
	m_detonated = true;
	m_body_sh->setSpriteIndex(1);
	m_explosion_sh->turnOn();
}

void CEagle::setNormalState()
{
	m_body_sh->setSpriteIndex(0);
	m_detonated = false;
	m_explosion_sh->reset();
	m_explosion_sh->turnOff();
}

bool CEagle::isDetonated() const
{
	return m_detonated;
}

void CEagle::update(int delta_time) 
{
	CGameObject::update(delta_time);
}

void CEagle::draw(sf::RenderWindow* render_window)
{
	m_body_sh->setPosition(getPosition());
	m_body_sh->draw(render_window);

	if (m_detonated)
	{
		m_explosion_sh->setPosition(getPosition());
		m_explosion_sh->draw(render_window);
	}
}

//------------------------------------------------------------------------------------------------------

CBattleCityGameScene::CBattleCityGameScene()
{
	setName("GameScene");

	srand(time(0));
	addObject(new Timer());
	addObject(m_walls = new CMap(BattleCityConsts::MAP_SIZE.x, BattleCityConsts::MAP_SIZE.y));
	addObject(m_player = new CTankPlayer(m_walls));

	addObject(m_eagle = new CEagle());
	m_eagle->setPosition(m_walls->toPixelCoordinates(BattleCityConsts::EAGLE_TILE));

	m_float_text = new CFlowText(*CBattleCityGame::instance()->fontManager().get("menu_font"));
	m_float_text->setTextColor(sf::Color::Yellow);
	m_float_text->setTextSize(25);
	addObject(m_float_text);
	
	m_bar_background = new CLabel();
	m_bar_background->setFillColor(sf::Color(212, 212, 212));
	m_bar_background->setBounds(700, 0, 120, 700);
	addObject(m_bar_background);

	sf::Sprite sprite(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { 250,150,25,25 });
	m_enemy_tanks_bar = new LifeBar(sprite, 2, m_tanks_on_level /2);
	m_enemy_tanks_bar->setPosition(735, 100);
	addObject(m_enemy_tanks_bar);

	m_score_label = new CLabel();
	m_score_label->setFontName(*CBattleCityGame::instance()->fontManager().get("menu_font"));
	m_score_label->setBounds(715, 430, 95, 20);
	m_score_label->setFontColor(sf::Color::Black);
	m_score_label->setFontSize(13);
	m_score_label->setOutlineColor(sf::Color::Black);
	m_score_label->setOutlineThickness(1);
	addObject(m_score_label);


	m_lifes_label = m_score_label->clone();
	m_lifes_label->setBounds(715,460,95,20);
	addObject(m_lifes_label);

	m_stage_label = m_score_label->clone();
	m_stage_label->setBounds(715, 490, 95, 20);
	addObject(m_stage_label);
	
	m_game_over_label = m_score_label->clone();
	m_game_over_label->setPosition({ 300,830 });
	m_game_over_label->setFontSize(50);
	m_game_over_label->setFontColor(sf::Color::White);
	m_game_over_label->setString("Game Over");
	m_game_over_label->setFillColor(sf::Color::Transparent);
	m_game_over_label->setOutlineThickness(0);
	m_game_over_label->hide();
	addObject(m_game_over_label);

	m_curtains = new CCurtains(Rect(0, 0, 25 * 28, 25 * 28), *CBattleCityGame::instance()->fontManager().get("menu_font"));
	addObject(m_curtains);

	reset();
}

void CBattleCityGameScene::loadStage(int index)
{	
	m_walls->getMap()->loadFromFile({ { '.',ETiles::empty },{ 'B',ETiles::brick },{ 'A',ETiles::armor },{ 'X',ETiles::border },{ 'W',ETiles::wood },{ 'L',ETiles::lake } }, "res/bs_stage"+toString(index)+".txt");
	m_walls->HPA_Finder().build(m_walls->getMap(), 8, 2);
	m_stage_label->setString("Stage " + toString(m_stage_index));

	addObject(new CHPAVisualiser(m_walls));
}

void CBattleCityGameScene::reset()
{
	m_score_label->setString("Score: 0");
	m_score = 0;
	m_player_tanks_lifes = 3;
	m_lifes_label->setString("Lifes: 3");
	
	m_enemy_spawn_counter = 0;
	m_enemy_crash_counter = 0;
	m_enemy_spawn_timer = 0;
	m_need_next_level_state = 0;

	m_eagle->setNormalState();
	m_stage_index = 0;
	m_need_next_level_state = 1;
	m_next_level_timer = 4500;
	
	m_need_game_over_state = 0;
	m_game_over_timer = 0;
	m_dy = 200;
	m_game_over_label->hide();

	m_player->setRank(0);
	m_player->enable();

	auto bonuses = findObjectsByType<CBonus>();
	for (auto bonus : bonuses)
		bonus->reset();

	auto enemy_tanks = findObjectsByType<CEnemyTank>();
	for (auto tank : enemy_tanks)
		removeObject(tank);

	auto bullets = findObjectsByType<CBullet>();
	for (auto bullet : bullets)
		removeObject(bullet);
}

CEnemyTank* CBattleCityGameScene::spawnEnemyTank()
{
	static std::pair<int, CEnemyTank::Type>  m_tanks_table[16][4] = 
         { 
			 { {18,CEnemyTank::Type::basic}, {2, CEnemyTank::Type::fast }  },
			 { {2,CEnemyTank::Type::armor }, {4, CEnemyTank::Type::fast }, {14,CEnemyTank::Type::basic }  },
			 { {14,CEnemyTank::Type::basic}, {4, CEnemyTank::Type::fast }, {2, CEnemyTank::Type::armor }  },
			 { {10,CEnemyTank::Type::power}, {5, CEnemyTank::Type::fast }, {2, CEnemyTank::Type::basic }, {3, CEnemyTank::Type::armor} },
			 { {5,CEnemyTank::Type::power }, {2, CEnemyTank::Type::armor}, {8, CEnemyTank::Type::basic }, {5, CEnemyTank::Type::fast } },
			 { {7,CEnemyTank::Type::power }, {2, CEnemyTank::Type::fast }, {9, CEnemyTank::Type::basic }, {2, CEnemyTank::Type::fast } },
	     };
 
	    int i = 0;
		CEnemyTank::Type tank_type;
		for (auto& pair : m_tanks_table[m_stage_index - 1])
		{
			i += pair.first;
			if (i > m_enemy_spawn_counter)
			{
				tank_type = pair.second;
				break;
			}
		}

	m_enemy_spawn_counter++;
	CEnemyTank* enemy_tank = new CEnemyTank(m_walls, m_player, tank_type);
	addObject(enemy_tank);
	enemy_tank->stop();
	enemy_tank->setState(CTank::EState::borning);
	enemy_tank->setPosition(m_walls->toPixelCoordinates(BattleCityConsts::ENEMY_SPAWN_TILES[m_enemy_spawn_counter % 3]));
	enemy_tank->setDirection(Vector::down);
	return enemy_tank;
}

void CBattleCityGameScene::spawnPlayerTank()
{
	m_player->spawn(m_walls->toPixelCoordinates(BattleCityConsts::PLAYER_SPAWN_TILE), Vector::up);
}

void CBattleCityGameScene::addScore(int score)
{
	m_score += score;
	m_score_label->setString("Score: " + toString(m_score));
}

void CBattleCityGameScene::addLifeToPlayerTank()
{
	m_player_tanks_lifes++;
	m_lifes_label->setString("Lifes: " + toString(m_player_tanks_lifes));
	CBattleCityGame::instance()->playSound("1-up");
}

void CBattleCityGameScene::removeLifeFromPlayerTank()
{
	m_player_tanks_lifes--;
	m_lifes_label->setString("Lifes: " + toString(m_player_tanks_lifes));
}

void CBattleCityGameScene::blowupAllTanks()
{
	auto enemy_tanks =  findObjectsByType<CEnemyTank>();
	for (auto enemy_tank : enemy_tanks)
	{
		if (enemy_tank->isAlive())
		{
			enemy_tank->detonate();
			m_enemy_tanks_bar->decrease();
			m_enemy_crash_counter++;
		}
	}
}

CBonus* CBattleCityGameScene::getRandomBonus()
{
	CBonus* bonus = NULL;

	int n = std::rand() % 6;

	switch (n)
	{
	  case 0:
	  {
		bonus = new CGrenede();
		break;
	  }
	  case 1:
	  {
		bonus = new CFreezer();
		break;
	  }
	  case 2:
	  {
		  bonus = new CHelmet();
		  break;
	  }
	  case 3:
	  {
		  bonus = new CShovel();
		  break;
	  }
	  case 4:
	  {
		  bonus = new CStar();
		  break;
	  }
	  case 5:
	  {
		  bonus = new CLife();
		  break;
	  }
	}

	return bonus;
}

void CBattleCityGameScene::hideHUD()
{
	m_enemy_tanks_bar->hide();
	m_score_label->hide();
	m_lifes_label->hide();
	m_stage_label->hide();
	m_bar_background->setFillColor({ 180,180,180 });
}

void CBattleCityGameScene::showHUD()
{
	m_enemy_tanks_bar->show();
	m_score_label->show();
	m_lifes_label->show();
	m_stage_label->show();
	m_bar_background->setFillColor({ 212,212,212 });
}

void CBattleCityGameScene::update(int delta_time)
{
	CGameObject::update(delta_time);

	//TANKS BORNING PROCESSING
	m_enemy_spawn_timer += delta_time;
	
	if (m_enemy_spawn_timer > BattleCityConsts::ENEMY_SPAWN_TIME)
	{
		if (findObjectsByType<CEnemyTank>().size() < 4 && m_enemy_spawn_counter < m_tanks_on_level)
		{
			CEnemyTank* tank = spawnEnemyTank();
			if (m_enemy_spawn_counter % BattleCityConsts::BONUS_TANKS_ST == 0)
				tank->setFlashed(true);
		}
		m_enemy_spawn_timer = 0;
	}
	 

	auto tanks = findObjectsByType<CTank>(); 

	foreachObject([this, &tanks](CGameObject* obj)
	{
		//BONUS PICKUP PROCESSING
		if (obj->getName() == "Bonus" && !obj->castTo<CBonus>()->isPickuping())
		{
			if (obj->getBounds().isIntersect(m_player->getBounds()))
			{
				obj->castTo<CBonus>()->pickup(m_player);
				m_float_text->splash(obj->getBounds().center(), "+500");
				addScore(500);
			}
		}

		if (obj->getName() == "Bullet" && !obj->castTo<CBullet>()->isDetonated())
		{
			enum class Endstatus { none, block_broken, armor_push, player_detonate, enemy_detonate, bullet_bullet, damage } end_status = Endstatus::none;

			CBullet* bullet = obj->castTo<CBullet>();
			bool armored = bullet->isArmorPiercing();

			//BULLETS CRASH WALLS PROCESSING
			Vector pos_a = m_walls->toMapCoordinates(obj->getBounds().center() + rotateClockwise(obj->getDirection() * 10));
			Vector pos_b = m_walls->toMapCoordinates(obj->getBounds().center() - rotateClockwise(obj->getDirection() * 10));

			for (auto& pos : { pos_a, pos_b })
			{
				if (m_walls->getMap()->inBounds(pos))
				{
					ETiles brick_type = m_walls->getMap()->getCell(pos);

					if (brick_type == ETiles::brick || brick_type == ETiles::armor || brick_type == ETiles::border)
					{
						if (brick_type == ETiles::brick || (brick_type == ETiles::armor && bullet->isArmorPiercing()))
						{
							m_walls->getMap()->setCell(pos.x, pos.y, ETiles::empty);
							end_status = Endstatus::block_broken;
						}
						else
						{
							end_status = Endstatus::armor_push;
						}
						break;
					}
				}
				else
				{
					end_status = Endstatus::armor_push;
					break;
				}
			}

			//BULLETS CRASH TANKS PROCESSINGH
			for (auto& tank : tanks)
			{
				if (tank->getBounds().isContain(obj->getBounds().center()))
				{
					if (tank != m_player) //enemy's tank
					{
						if (bullet->source() == m_player && tank->isAlive())
						{
							tank->damage();
							
							if (tank->castTo<CEnemyTank>()->isFlashing())
							{
								CBonus* bonus = getRandomBonus();
								Vector bonus_tile(1 + std::rand() % int(BattleCityConsts::MAP_SIZE.x - 2), 1 + std::rand() % int(BattleCityConsts::MAP_SIZE.y - 2));
								bonus->setPosition(m_walls->toPixelCoordinates(bonus_tile));
								addObject(bonus);
								tank->castTo<CEnemyTank>()->setFlashed(false);
							}
							int score = ((int)tank->castTo<CEnemyTank>()->type() + 1) * 100;
							if (tank->isDetonated())
							{
								m_float_text->splash(tank->getBounds().center(), "+" + toString(score));
								m_enemy_tanks_bar->decrease();
								m_enemy_crash_counter++;
								addScore(score);
								end_status = Endstatus::enemy_detonate;
							}
							else
							{
								end_status = Endstatus::damage;
							}
							break;
						}
					}
					else // player's tank
					{
						if (tank->isAlive())
						{
							tank->damage();
							end_status = Endstatus::player_detonate;

							if (tank->isDetonated())
							{
								if (m_player_tanks_lifes > 0)
								{
									removeLifeFromPlayerTank();
									findObjectByName<Timer>("Timer")->add(sf::seconds(1), [this]() { spawnPlayerTank(); });
								}
								else
								{
									m_need_game_over_state = 1;
								}
							}
						}
					}
				}
			}

			//BULLETS CRASH EAGLE PROCESSING
			if (!m_eagle->isDetonated())
			{
				if (m_eagle->getBounds().isContain(obj->getPosition()))
				{
					m_eagle->detonate();
					end_status = Endstatus::player_detonate;
					m_need_game_over_state = 1;
					if (m_player->isAlive())
					{
						m_player->disable();
					}
				}
			}

			switch (end_status)
			{

				case(Endstatus::armor_push):
				{
					if (bullet->source() == m_player)
						CBattleCityGame::instance()->playSound("armor-push");
					break;
				}
				case(Endstatus::block_broken):
				{
					if (bullet->source() == m_player)
						CBattleCityGame::instance()->playSound("block-broken");
					break;
				}
				case(Endstatus::enemy_detonate):
				{
					if (bullet->source() == m_player)
						CBattleCityGame::instance()->playSound("enemy-boom");
					break;
				}
				case(Endstatus::damage):
				{
					if (bullet->source() == m_player)
						CBattleCityGame::instance()->playSound("damage");
					break;
				}
				case(Endstatus::player_detonate):
				{
					if (m_player->isAlive())
					{
						CBattleCityGame::instance()->playSound("damage");
					}
					else
					{
						CBattleCityGame::instance()->playSound("player-boom");
					}
					break;
				}
			}

			if (end_status != Endstatus::none)
			{
				bullet->detonate();
			}

			//Bullets crash bullets
			foreachObject([obj,this](CGameObject* obj2, bool& need_break)
			{
				if (obj2->getName() == "Bullet")
				{
					CBullet* bullet_one = obj->castTo<CBullet>();
					CBullet* bullet_two = obj2->castTo<CBullet>();

					if (obj != obj2)
					{
						need_break = false;
						if ((bullet_one->getBounds().center() - bullet_two->getBounds().center()).length() < 10
							&& !bullet_one->isDetonated() && !bullet_two->isDetonated())
						{
							bullet_one->detonate(true);
							bullet_two->detonate(true);
						}
					}
					else
					{
						need_break = true;
					}
				}
			});

		}
	});

	// TANKS COLLISION PROCESSING
	static const auto setOldPosition = [delta_time](CTank* tank)
	{
		tank->setPosition(tank->getPosition() - delta_time*tank->getSpeed()*tank->getDirection());
		tank->setSpeed(0);
	 	tank->stop();
	};
	
	for (int i = 0; i < tanks.size(); ++i)
	{
		auto tank_one = tanks[i];

		if (tank_one->isDetonated())
			continue;

		// tank vs tanks colliding
		for (int j = i + 1; j < tanks.size(); ++j)
		{
			auto tank_two = tanks[j];

			if (tank_two->isDetonated())
				continue;

			if (tank_one->getBounds().isIntersect(tank_two->getBounds()))
			{
				//Collision response processing: who ñauses collision must stopped

				if (tank_one->getDirection() == -tank_two->getDirection()) // both tanks cause
				{
					setOldPosition(tank_one);
					setOldPosition(tank_two);
				}
				else
				{
					Vector own_tank_center = tank_one->getBounds().center();
					Vector other_tank_center = tank_two->getBounds().center();
					Vector own_tank_bamper = own_tank_center + 25 * tank_one->getDirection();
					Vector other_tank_bamper = other_tank_center + 25 * tank_two->getDirection();
					Vector coll_center = (own_tank_center + other_tank_center) / 2;
					if ((own_tank_bamper - coll_center).length() < (other_tank_bamper - coll_center).length())  //first tank cause
					{
						setOldPosition(tank_one);
					}
					else
					{
						setOldPosition(tank_two);
					}
				}
			}
		}

		// tank vs walls colliding
		Vector future_pos = tank_one->getPosition() + delta_time * tank_one->getSpeed() * tank_one->getDirection();
		Rect future_bounds = Rect(future_pos, tank_one->getBounds().size());
		if (m_walls->isCollide(future_bounds, { ETiles::empty, ETiles::wood }))
		{
			tank_one->stop();
			tank_one->setPosition(m_walls->alignToTiles(tank_one->getPosition()));
		}

		// tanks vs Eagle colliding
		if (m_eagle->getBounds().isIntersect(tank_one->getBounds()))
		{
			setOldPosition(tank_one);
		}
	}
	
	//GO TO NEXT LEVEL PROCESSING
	if (m_enemy_crash_counter >= m_tanks_on_level && m_need_next_level_state == 0)
	{
		m_need_next_level_state = 1;
		m_next_level_timer = 0;
	}

	if (m_need_next_level_state > 0)
	{
		m_next_level_timer += delta_time;

		if (m_need_next_level_state == 1 && m_next_level_timer > 3000)
		{
			m_stage_index++;
			m_curtains->play("Stage " + toString(m_stage_index), m_stage_index == 1 ? true : false);
			m_need_next_level_state = 2;
			hideHUD();
		}	else
		if (m_need_next_level_state == 2 && m_next_level_timer > 4500)
		{
			m_player->hide();
			loadStage(m_stage_index);
			m_need_next_level_state = 3;
			CBattleCityGame::instance()->playSound("stage_start");
		}	else
		if (m_need_next_level_state == 3 && m_next_level_timer > 6500)
		{
			m_enemy_spawn_counter = m_enemy_spawn_timer = m_enemy_crash_counter = 0;
			m_need_next_level_state = 0;
			m_enemy_tanks_bar->setValue(m_tanks_on_level);
			showHUD();
			spawnPlayerTank();
			m_player->show();
		}	
	}

	// GAME OVER PROCESSING
	if (m_need_game_over_state > 0)
	{
		m_game_over_timer += delta_time;

		if (m_need_game_over_state == 1)
		{
			m_game_over_label->show();
			m_need_game_over_state = 2;
		}	else
		if (m_need_game_over_state == 2 && m_dy > 0)
		{
			m_game_over_label->setPosition(Vector(300.f, 330.f + m_dy * delta_time / 5));
			m_dy--;
		}	else 
		if (m_game_over_timer > 4000)
		{
			auto menu = getParent()->findObjectByName<CBattleCityMenuScene>("MenuScene");
			menu->reset();
			menu->turnOn();
			turnOff();
		}
	}
}

//----------------------------------------------------------------------------------------------------------

CBattleCityMenuScene::CBattleCityMenuScene()
{
	setName("MenuScene");

	m_logo_label = new CLabel();
	m_logo_label->setSprite(sf::Sprite(*CBattleCityGame::instance()->textureManager().get("battle_city_logo"), { 0,0,800,205 }));
	m_logo_label->setPosition({ 10,50 });
	addObject(m_logo_label);

	m_one_player_label = new CLabel("One player");
	m_one_player_label->setFontName(*CBattleCityGame::instance()->fontManager().get("menu_font"));
	m_one_player_label->setFontColor(sf::Color::White);
	addObject(m_one_player_label);

	m_two_player_label = m_one_player_label->clone();
	m_two_player_label->setString("Two players");
	addObject(m_two_player_label);

	m_constructor_label = m_one_player_label->clone();
	m_constructor_label->setString("Constructor");
	addObject(m_constructor_label);

	m_about_label = new CLabel("A. Parfenyuk. 2017");
	m_about_label->setFontColor(sf::Color::White);
	m_about_label->setFontStyle(sf::Text::Bold);
	m_about_label->setFontSize(17);
	m_about_label->setFontName(*CBattleCityGame::instance()->fontManager().get("menu_font"));
	addObject(m_about_label);

	m_cursor_label = new CLabel();
	m_cursor_label->setSprite(sf::Sprite(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { 50,50,50,50 }));
	addObject(m_cursor_label);

	reset();
}

void CBattleCityMenuScene::update(int delta_time)
{
	CGameObject::update(delta_time);

	auto intput_manager = CBattleCityGame::instance()->inputManager();
	auto input = intput_manager.getXYAxis();


	if (m_cursor_label->isVisible())
	{
		if (!m_prev_input.y && input.y)
		{
			if (input.y > 0)
			{
				m_cursor_pos++;
				if (m_cursor_pos > 2) 
					m_cursor_pos = 0;
			}
			else 
			{
				m_cursor_pos--;
				if (m_cursor_pos < 0) 
					m_cursor_pos = 2;
			}
			m_cursor_label->setPosition(230, 320 + m_cursor_pos * 60);			
		}

		if (intput_manager.isButtonDown("Fire"))
		{
			if (m_cursor_pos == 0)
			{
				auto game_scene = getParent()->findObjectByName<CBattleCityGameScene>("GameScene");
				game_scene->reset();
				game_scene->turnOn();
				this->turnOff();
			}
		}
	}
	else
	{
		if (intput_manager.isButtonDown("Fire"))
		{
			m_dy = 1;
		}
	}

	if (m_dy == 200)
	{
		m_logo_label->show();
		m_one_player_label->show();
		m_two_player_label->show();
		m_constructor_label->show();
		m_about_label->show();
	}
	if (m_dy > 0)
	{
		int dy = m_dy*delta_time / 5;
		m_logo_label->setPosition({ 10,dy + 50 });
		m_one_player_label->setPosition({ 400,dy + 340 });
		m_two_player_label->setPosition({ 400,dy + 400 });
		m_constructor_label->setPosition({ 400,dy + 460 });
		m_about_label->setPosition({ 390,dy + 680 });
		m_dy--;
	}
	if (m_dy == 0)
	{
		m_cursor_label->show();
	}

	m_prev_input = input;
}

void CBattleCityMenuScene::reset()
{
	m_cursor_pos = 0;
	m_dy = 200;
	m_cursor_label->setPosition(230, 320);
	m_logo_label->hide();
	m_one_player_label->hide();
	m_two_player_label->hide();
	m_constructor_label->hide();
	m_about_label->hide();
	m_cursor_label->hide();
}

//----------------------------------------------------------------------------------------------------------

CMap::CMap(int width, int height):
	m_map(width,height),
	m_HPA_finder(ALLOWED_CELL_PREDICATE)
{
	setName("Map");
	m_sprite_sheet.load(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { { 0,0,25,25 },{ 25,0,25,25 },{ 50,0,25,25 }, {75,0,25,25}, {00,25,25,25} });
	
	m_eagle_sprite.setTexture(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"));
	m_eagle_sprite.setTextureRect({ 100,0,50,50 });
	m_eagle_sprite.setPosition(toPixelCoordinates(BattleCityConsts::EAGLE_TILE));

	m_shape.setSize(Vector(m_map.width(), m_map.height())*tile_size);
	m_shape.setFillColor(sf::Color(12,12,12));
}

void CMap::draw(sf::RenderWindow* render_window)
{
	render_window->draw(m_shape);

	for (int x = 0; x < m_map.width(); ++x)
		for (int y = 0; y < m_map.height(); ++y)
		{
			int i = m_map.getCell(x, y) - 1;
			if (i < 0)
				continue;
			m_sprite_sheet.setPosition(sf::Vector2f(x*tile_size, y*tile_size));
			render_window->draw(m_sprite_sheet[i]);
		}	
}

void CMap::postDraw(sf::RenderWindow* render_window)
{
	for (int x = 0; x < m_map.width(); ++x)
		for (int y = 0; y < m_map.height(); ++y)
		{
			int i = m_map.getCell(x, y) - 1;
			if (m_map.getCell(x, y) == ETiles::wood )
			{
				m_sprite_sheet.setPosition(sf::Vector2f(x*tile_size, y*tile_size));
				render_window->draw(m_sprite_sheet[i]);
			}
		}
}

Rect CMap::toPixelCoordinates(const Rect& rect)
{
	return rect*tile_size;
}

bool CMap::isCollide(Rect& rect, const std::vector<ETiles>& allowed_cell_types)
{
	Vector lt = toMapCoordinates(rect.leftTop());
	Vector rb = toMapCoordinates(rect.rightBottom());

	for (int x = lt.x; x < rb.x; ++x)
		for (int y = lt.y; y < rb.y; ++y)
		{ 
			bool dissalowed = true;
			for (auto& cell_type : allowed_cell_types)
			{
				if (m_map.getCell(x, y) == cell_type)
				{
					dissalowed = false;
					break;
				}
			}
			
			if (dissalowed)
			{
				return true;
			}
		}

	return false;
}

void CMap::update(int delta_time)
{
	m_timer += delta_time;
	 
	int i = (m_timer / 1000) % 3;

	if (m_water_index != i)
	{
		m_water_index = i;
		m_sprite_sheet[4].setTextureRect(sf::IntRect(m_water_index * 25, 25, 25, 25));
	}

	if (m_timer > 5000)
	{
		std::async([this] 
			{
			//	hpa_blocked = true;  
				m_HPA_finder.update();
			//	hpa_blocked = false; 
			});
		m_timer = 0;
	
#ifdef VISUAL_DEBUG
		getParent()->findObjectByName<CHPAVisualiser>("HPAVisualiser")->refresh();
#endif // VISUAL_DEBUG

	}
}

TileMap<ETiles>* CMap::getMap()
{
	//while (hpa_blocked);

	return &m_map;
}

Vector CMap::toPixelCoordinates(const Vector& point)
{
	return point*tile_size;

}

std::vector<Vector> CMap::toPixelCoordinates(const std::vector<Vector>& points)
{
	std::vector<Vector> res(points.size());

	for (int i = 0; i < points.size(); ++i)
		res[i] = points[i] * tile_size;

	return res;
}

Vector CMap::toMapCoordinates(const Vector& point, bool rounded)
{
	if (rounded)
	{
		return round(point / tile_size);
	}
	return  point/tile_size;
}

Vector CMap::alignToTiles(const Vector& pos)
{
	return round(pos / tile_size) * tile_size;
}

HPA_Finder<ETiles>& CMap::HPA_Finder()
{
	return m_HPA_finder;
}

//------------------------------------------------------------------------------------------------------------

LifeBar::LifeBar(const sf::Sprite& life_sprite, int cols, int rows)
{
	m_value = rows*cols;
	m_rows = rows; m_cols = cols;
	m_background_color = sf::Color::Transparent;
	m_life_sprite = life_sprite;
}

void LifeBar::setBackgroundColor(const sf::Color& color)
{
	m_background_color = color;
}

void LifeBar::setValue(int value)
{
	assert(value > 0 && value <= m_rows*m_cols);
	m_value = value ;
}

void LifeBar::decrease()
{
	--m_value;
}

void LifeBar::draw(sf::RenderWindow* render_window) 
{
	int row = 0;
	int col = 0;
	int size = 28;// m_life_sprite.getTextureRect().width;

	for (int i = 0; i < m_value; ++i)
	{
		int row =  i / m_cols;
		int col =  i % m_cols;
		Vector local_pos = Vector(col, row)*size;
		
		m_life_sprite.setPosition(local_pos + getPosition());
		render_window->draw(m_life_sprite);
	}
}

//-------------------------------------------------------------------------------------------------------------------

CCurtains::CCurtains(const Rect& rect, const sf::Font& font)
{
	setBounds(rect);
	m_up_curtain.setPosition({ 0,0 });
	m_up_curtain.setSize({ rect.width(),rect.height() / 2 });

	m_down_curtain.setPosition({ 0,rect.height() / 2 });
	m_down_curtain.setSize({ rect.width(),rect.height() / 2 });

	m_up_curtain.setFillColor(sf::Color(180, 180, 180));
	m_down_curtain.setFillColor(sf::Color(180, 180, 180));
	m_label = new CLabel();
	m_label->setFontName(font);
	m_label->setPosition(rect.center());

	m_shadow.setPosition({ 0,0 });
	m_shadow.setSize(rect.size());
	m_shadow.setFillColor(sf::Color::Black);

	m_state = 3;
}

CCurtains::~CCurtains()
{
	delete m_label;
}

void CCurtains::play(const std::string& text, bool shadowed)
{
	m_shadowed = shadowed;
	m_timer = 0;
	m_state = 0;
	m_label->setString(text);
}

void CCurtains::postDraw(sf::RenderWindow* render_window)
{
	if (m_shadowed && m_state == 0)
	{
		render_window->draw(m_shadow);
	}

	render_window->draw(m_up_curtain);
	render_window->draw(m_down_curtain);
	
	if (m_timer > 200 && m_timer < 800)
	{
		m_label->draw(render_window);
	}
}

void CCurtains::update(int delta_time)
{
	CGameObject::update(delta_time);
	auto size = getBounds().size();

	//States
	switch (m_state)
	{
		case(0):
		{
			m_h += 0.3 * delta_time;
			if (m_h > (size.y / 2))
			{
				m_state = 1;
			}
			break;
		}
		case(1):
		{
			m_timer += delta_time;
			if (m_timer > 1000) 
			{ 
				m_timer = 0; 
				m_state = 2; 
			}
			break;
		}
		case(2):
		{
			m_h -= 0.3 * delta_time;
			if (m_h < 0)
			{
				m_state = 3;
			}
			break;
		}
	}
 
	if (m_state != 3)
	{
		m_up_curtain.setPosition(sf::Vector2f(0, -size.y / 2 + m_h));
		m_down_curtain.setPosition(0, size.y - m_h);
	}
}

//-------------------------------------------------------------------------------------------------------------------

CHPAVisualiser::CHPAVisualiser(CMap* map) :
	m_HPA_Finder(ALLOWED_CELL_PREDICATE)
{
	setName("HPAVisualiser");
	m_map = map;
	sf::Clock clock;
}

void CHPAVisualiser::refresh()
{
	m_HPA_Finder.build(m_map->getMap(), 8, 2);
	m_path = m_HPA_Finder.search({ 1,24 }, { 25,1 });
}

void CHPAVisualiser::draw(sf::RenderWindow* render_window)
{
	//Clasters 
	sf::RectangleShape shape;
	shape.setOutlineColor(sf::Color::Red);
	shape.setFillColor(sf::Color::Transparent);
	shape.setOutlineThickness(2);

	for (auto& r : m_HPA_Finder.m_clasters)
	{
		shape.setPosition(m_map->toPixelCoordinates(r.leftTop()));
		shape.setSize(m_map->toPixelCoordinates(r.size()));
		render_window->draw(shape);
	}

	//Transition-Points
	shape.setOutlineColor(sf::Color::Black);
	shape.setFillColor(sf::Color::Green);
	shape.setOutlineThickness(1);

	shape.setSize(m_map->toPixelCoordinates({ 1,1 }));
	for (auto& ls : m_HPA_Finder.m_trans_points)
		for (auto& p : ls.second)
		{
			shape.setPosition(m_map->toPixelCoordinates(p));
			render_window->draw(shape);
		}

	//Path 
	//shape.setFillColor(sf::Color::White);
	sf::VertexArray vertexes(sf::PrimitiveType::LineStrip);

	for (auto& p : m_path)
	{
		vertexes.append(sf::Vertex(m_map->toPixelCoordinates(p + Vector(0.5f, 0.5f))));
	}

	render_window->draw(vertexes);
}