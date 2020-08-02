#ifndef  PICKUPS_H
#define PICKUPS_H

#include "GameEngine/GameEngine.h"

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

class CMap;
enum ETiles;

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

#endif
