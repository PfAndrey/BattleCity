#include "Pickups.h"
#include "BattleCityGame.h"

CBonus::CBonus()
{
	setName("Bonus");
	setSize({ size, size });
}

void CBonus::postDraw(sf::RenderWindow* render_window)
{
	m_sprite.setPosition(getPosition());
	render_window->draw(m_sprite);
	float color = 200 + 50 * cos(m_timer / 150);
	m_sprite.setColor(sf::Color(color, color, color));
}

void CBonus::update(int delta_time)
{
	m_timer += delta_time;

	if (!isPickuping() && m_timer > BattleCityConsts::BONUS_TIME_ALIVE)
	{
		getParent()->removeObject(this);
	}
}

void CBonus::pickup(CTank* pickuper)
{
	m_pickuper = pickuper;
	if (!isTypeOf<CLife>())
	{
		CBattleCityGame::instance()->playSound("bonus-picked");
	}
}

bool CBonus::isPickuping() const
{
	return m_pickuper != nullptr;
}

void CBonus::setSprite(const sf::Sprite& sprite)
{
	m_sprite = sprite;
}

int CBonus::getTime() const
{
    return m_timer;
}

void CBonus::resetTime()
{
	m_timer = 0;
}

void CBonus::reset()
{
	getParent()->removeObject(this);
}

//------------------------------------------------------------------------------------------------------------

CGrenede::CGrenede()
{
	sf::Sprite sprite(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { 200,0,50,50 });
	setSprite(sprite);
}

void CGrenede::update(int delta_time) 
{
	CBonus::update(delta_time);

	if (isPickuping())
	{
		getParent()->castTo<CBattleCityGameScene>()->blowupAllTanks();
		getParent()->removeObject(this);
	}
}

//------------------------------------------------------------------------------------------------------------

CFreezer::CFreezer()
{
	sf::Sprite sprite(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { 250,0,50,50 });
	setSprite(sprite);
}

void CFreezer::update(int delta_time) 
{
	CBonus::update(delta_time);

	if (isPickuping())
	{
		if (m_step == 0)
		{
			this->hide();
			auto enemy_tanks = getParent()->findObjectsByType<CEnemyTank>();
			for (auto enemy_tank : enemy_tanks)
			{
				enemy_tank->stop();
			}
			CEnemyTank::setFreezed(true);
			m_step++;
			resetTime();
		}
		else if (m_step == 1 && getTime() > BattleCityConsts::TIME_OF_FREEZING)
		{
			auto enemy_tanks = getParent()->findObjectsByType<CEnemyTank>();
			for (auto enemy_tank : enemy_tanks)
				enemy_tank->setFreezed(false);

			getParent()->removeObject(this);
			m_step++;
		}
	}
}

void CFreezer::reset()
{
	CBonus::reset();
	CEnemyTank::setFreezed(false);
}

//-------------------------------------------------------------------------------------------------------------

CHelmet::CHelmet()
{

	sf::Sprite sprite(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { 250,50,50,50 });
	setSprite(sprite);
}

void CHelmet::update(int delta_time) 
{
	CBonus::update(delta_time);

	if (isPickuping())
	{
		m_pickuper->turnOnShield(BattleCityConsts::TIME_OF_HELMET);
		getParent()->removeObject(this);
	}
}

void CHelmet::reset()
{
	CBonus::reset();
	m_pickuper->turnOffShield();
}

//-------------------------------------------------------------------------------------------------------------

CShovel::CShovel()
{
	sf::Sprite sprite(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { 200,50,50,50 });
	setSprite(sprite);

	const Vector& eagle_cell = BattleCityConsts::EAGLE_TILE;

	static const Vector deltas[] = { { -1,1 },{ -1,0 },{ -1,-1 },{ 0,-1 },{ 1,-1 },{ 2,-1 },{ 2,0 },{ 2,1 } };

	for (auto& delta : deltas)
		m_cells.push_back(eagle_cell + delta);

}

void CShovel::fence(const ETiles& tile)
{
	for (auto& cell : m_cells)
		m_map->getMap()->setCell(cell.x, cell.y, tile);
}

void CShovel::onActivated()
{
	m_map = getParent()->findObjectByName("Map")->castTo<CMap>();
}

void CShovel::update(int delta_time)
{
	CBonus::update(delta_time);

	if (isPickuping())
	{
		if (m_step == 0)
		{
			this->hide();
			fence(ETiles::armor);
			m_step++;
			resetTime();
		}
		else if (m_step == 1 && getTime() > BattleCityConsts::TIME_OF_SHOVEL - 3000) // warning
		{
			if (int(getTime() / 400) % 2)
			{
				fence(ETiles::armor);
			}
			else
			{
				fence(ETiles::brick);
			}

			if (getTime() > BattleCityConsts::TIME_OF_SHOVEL)
			{
				m_step++;
			}
		}
		else if (m_step == 2)
		{
			fence(ETiles::brick);
			getParent()->removeObject(this);
			m_step++;
		}
	}
}

void CShovel::reset()
{
	CBonus::reset();
	fence(ETiles::brick);
}

//----------------------------------------------------------------------------------------

CStar::CStar()
{

	sf::Sprite sprite(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { 200,100,50,50 });
	setSprite(sprite);
}

void CStar::update(int delta_time)
{
	CBonus::update(delta_time);

	if (isPickuping())
	{
		m_pickuper->castTo<CTankPlayer>()->promote(); 
		getParent()->removeObject(this);
	}
}

//-------------------------------------------------------------------------------------------------------------------

CLife::CLife()
{
	sf::Sprite sprite(*CBattleCityGame::instance()->textureManager().get("battle_city_sheet"), { 250,100,50,50 });
	setSprite(sprite);
}

void CLife::update(int delta_time)
{
	CBonus::update(delta_time);

	if (isPickuping())
	{
		auto scene = (CBattleCityGameScene*)getParent();
		scene->addLifeToPlayerTank();
		getParent()->removeObject(this);
	}
}