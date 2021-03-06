#include "../headers/World.h"

World::SpawnPoint::SpawnPoint(Aircraft::Type type, float x, float y):
type(type),
x(x),
y(y)
{

}

World::World(sf::RenderTarget& outputTarget, FontHolder& fonts, SoundPlayer& sounds) : 
mTarget(outputTarget),
mFonts(fonts),
mSounds(sounds),
mWorldView(outputTarget.getDefaultView()),
mWorldBounds(0.f, 0.f, mWorldView.getSize().x, 5000.f),
mSpawnPosition(mWorldView.getSize().x/2.f, mWorldBounds.height - mWorldView.getSize().y /2.f),
mScrollSpeed(-50.f),
mPlayerAircraft(nullptr),
mEnemySpointPoints(),
mActiveEnemies(),
gameOver(false)
{
	mSceneTexture.create(outputTarget.getSize().x, outputTarget.getSize().y);

	loadTextures();
	buildScene();

	addEnemies();

	mWorldView.setCenter(mSpawnPosition);

}

void World::loadTextures(){

	mTextures.load(Resources::Textures::Entities, "Resources/img/Entities.png");
	mTextures.load(Resources::Textures::Jungle, "Resources/img/Jungle.png");
	mTextures.load(Resources::Textures::Particle, "Resources/img/Particle.png");
	mTextures.load(Resources::Textures::Explosion, "Resources/img/Explosion.png");
	mTextures.load(Resources::Textures::FinishLine, "Resources/img/FinishLine.png");

}

void World::buildScene(){

	//Initialize each layer
	for (std::size_t i = 0; i < LayerCount; ++i){

		Category::Type category = (i == LowerAir) ? Category::SceneAirLayer : Category::None;

		SceneNode::Ptr layer(new SceneNode(category));
		mSceneLayers[i] = layer.get();
		mSceneGraph.attachChild(std::move(layer));
	}


	//printf("%i\n", mSceneGraph.nbChildren());
	//Initialize desert sprite node
	sf::Texture& texture = mTextures.get(Resources::Textures::Jungle);
	sf::IntRect textureRect(mWorldBounds);
	texture.setRepeated(true);

	std::unique_ptr<SpriteNode> backgroundSprite(new SpriteNode(texture, textureRect));
	backgroundSprite->setPosition(mWorldBounds.left, mWorldBounds.top);
	mSceneLayers[Background]->attachChild(std::move(backgroundSprite));

	sf::Texture& finishTexture = mTextures.get(Resources::Textures::FinishLine);
	std::unique_ptr<SpriteNode> finishSprite(new SpriteNode(finishTexture));
	finishSprite->setPosition(0.f, -76.f);
	mSceneLayers[Background]->attachChild(std::move(finishSprite));

	//Initialize planes
	std::unique_ptr<Aircraft> leader(new Aircraft(Aircraft::EAGLE, mTextures, mFonts));
	mPlayerAircraft = leader.get();
	mPlayerAircraft->setPosition(mSpawnPosition);
	mPlayerAircraft->setVelocity(40.f, mScrollSpeed);
	mSceneLayers[UpperAir]->attachChild(std::move(leader));

	std::unique_ptr<Pickup> health(new Pickup(Pickup::HealthRefill, mTextures));
	health->setPosition(mSpawnPosition - sf::Vector2f(0.f,200.f));
	mSceneLayers[UpperAir]->attachChild(std::move(health));

	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(Particle::Smoke, mTextures));
	mSceneLayers[LowerAir]->attachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(Particle::Propellant, mTextures));
	mSceneLayers[LowerAir]->attachChild(std::move(propellantNode));

	std::unique_ptr<SoundNode> soundNode(new SoundNode(mSounds));
	mSceneGraph.attachChild(std::move(soundNode));
	/**std::unique_ptr<Aircraft> leftEscort(new Aircraft(Aircraft::RAPTOR, mTextures, mFonts));
	leftEscort->setPosition(-80.f, 50.f);
	mPlayerAircraft->attachChild(std::move(leftEscort));
	

	std::unique_ptr<Aircraft> rightEscort(new Aircraft(Aircraft::RAPTOR, mTextures, mFonts));
	rightEscort->setPosition(80.f,50.f);
	mPlayerAircraft->attachChild(std::move(rightEscort));**/

}

void World::draw(){

	if (PostEffect::isSupported()){

		mSceneTexture.clear();
		mSceneTexture.setView(mWorldView);
		mSceneTexture.draw(mSceneGraph);
		mSceneTexture.display();
		mBloomEffect.apply(mSceneTexture, mTarget);

	}

	else{
		mTarget.setView(mWorldView);
		mTarget.draw(mSceneGraph);
	}
}

void World::update(sf::Time dt){
	//printf("%i\n", mSceneGraph.getNodes().size());

	/*for (int i = 0; i < mSceneGraph.getNodes().size(); i++){
		SceneNode* n = mSceneGraph.getNode(i);
		if (n->getCategory() & Category::PlayerAircraft){
			auto p = static_cast<Aircraft*>(n);
			p->move(0.2f,0.f);
		}
			
	}
	*/


	mWorldView.move(0.f, mScrollSpeed*dt.asSeconds());
	mPlayerAircraft->setVelocity(0.f, 0.f);
	
	destroyEntitiesOutsideView();
	guideMissiles();

	//forward commands to the scene graph
	while (!mCommandQueue.isEmpty())
		mSceneGraph.onCommand(mCommandQueue.pop(), dt);
	adaptPlayerVelocity();

	handleCollisions();	
	mSceneGraph.removeWrecks();
	//mSceneGraph.clearNodes();
	spawnEnemies();

	mSceneGraph.update(dt, mCommandQueue);
	adaptPlayerPosition();

	updateSounds();
}

CommandQueue& World::getCommandQueue(){
	return mCommandQueue;
}

sf::FloatRect World::getViewBounds() const {
	return sf::FloatRect(mWorldView.getCenter() - mWorldView.getSize() / 2.f, mWorldView.getSize());
}

sf::FloatRect World::getBattlefieldBounds() const {
	sf::FloatRect bounds = getViewBounds();
	bounds.top -= 100.f;
	bounds.height += 100.f;

	return bounds;
}

void World::spawnEnemies(){
	while (!mEnemySpointPoints.empty() && mEnemySpointPoints.back().y > getViewBounds().top){

		SpawnPoint spawn = mEnemySpointPoints.back();
		
		std::unique_ptr<Aircraft> ennemy(new Aircraft(spawn.type, mTextures, mFonts));
		ennemy->setPosition(spawn.x, spawn.y);
		ennemy->setRotation(180.f);

		mSceneLayers[UpperAir]->attachChild(std::move(ennemy));

		mEnemySpointPoints.pop_back();
	}
}

void World::addEnemy(Aircraft::Type type, float x, float y){
	mEnemySpointPoints.push_back(SpawnPoint(type, mSpawnPosition.x + x, mSpawnPosition.y - y));
}

void World::addEnemies(){
	addEnemy(Aircraft::AVENGER, -100.f, 500.f);
	addEnemy(Aircraft::RAPTOR, 0.f, 1000.f);
	addEnemy(Aircraft::RAPTOR, +100.f, 1100.f);
	addEnemy(Aircraft::RAPTOR, -100.f, 1100.f);
	addEnemy(Aircraft::AVENGER, -70.f, 1400.f);
	addEnemy(Aircraft::AVENGER, -70.f, 1600.f);
	addEnemy(Aircraft::AVENGER, 70.f, 1400.f);
	addEnemy(Aircraft::AVENGER, 70.f, 1600.f);

	std::sort(mEnemySpointPoints.begin(), mEnemySpointPoints.end(), [](SpawnPoint lhs, SpawnPoint rhs){
		return lhs.y < rhs.y;
	});

}

void World::guideMissiles(){
	Command enemyCollector;
	enemyCollector.category = Category::EnemyAircraft;
	enemyCollector.action = derivedAction<Aircraft>(
		[this](Aircraft& enemy, sf::Time)
		{
			if (!enemy.isDestroyed())
				mActiveEnemies.push_back(&enemy);
		});

	Command missileGuider;
	missileGuider.category = Category::AlliedProjectile;
	missileGuider.action = derivedAction<Projectile>(
		[this](Projectile& missile, sf::Time)
		{
			//Ignore unguided bullets
			if (!missile.isGuided())
				return;

			float minDistance = std::numeric_limits<float>::max();
			Aircraft* closestEnemy = nullptr;

			FOREACH(Aircraft* enemy, mActiveEnemies){
				float enemyDistance = distance(missile, *enemy);

				if (enemyDistance < minDistance){
					closestEnemy = enemy;
					minDistance = enemyDistance;
				}

			}

			if (closestEnemy){
				missile.guideTowards(closestEnemy->getWorldPosition());
			}
		});

	mCommandQueue.push(enemyCollector);
	mCommandQueue.push(missileGuider);
	mActiveEnemies.clear();
}

void World::handleCollisions(){
	std::set < SceneNode::Pair > collisionPairs;
	mSceneGraph.checkSceneCollision(mSceneGraph, collisionPairs);
	//printf("%i\n", collisionPairs.size());

	FOREACH(SceneNode::Pair pair, collisionPairs){
		if (matchesCategories(pair, Category::PlayerAircraft, Category::EnemyAircraft)){
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& enemy = static_cast<Aircraft&>(*pair.second);

			player.damage(enemy.getHitPoints());
			if (player.isDestroyed())
				gameOver = true;
			enemy.destroy();
		}

		else if (matchesCategories(pair, Category::PlayerAircraft, Category::Pickup)){
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);

			pickup.apply(player);
			pickup.destroy();
			player.playLocalSound(mCommandQueue, Resources::SoundEffects::CollectPickup);
		}

		else if (matchesCategories(pair, Category::EnemyProjectile, Category::PlayerAircraft)){
			auto& projectile = static_cast<Projectile&>(*pair.first);
			auto& aircraft = static_cast<Aircraft&>(*pair.second);

			aircraft.damage(projectile.getDamage());
			if (aircraft.isDestroyed())
				gameOver = true;
			projectile.destroy();

		}

		else if (matchesCategories(pair, Category::AlliedProjectile, Category::EnemyAircraft)){
			auto& projectile = static_cast<Projectile&>(*pair.first);
			auto& aircraft = static_cast<Aircraft&>(*pair.second);

			aircraft.damage(projectile.getDamage());
			projectile.destroy();

		}
	}

}

void World::destroyEntitiesOutsideView(){
	Command command;
	command.category = Category::Projectile | Category::EnemyAircraft;
	command.action = derivedAction<Entity>([this](Entity& e, sf::Time){
		if (!getBattlefieldBounds().intersects(e.getBoundingRect()))
			e.remove();
	});

	mCommandQueue.push(command);
}

bool World::gameStatus() const{
	return gameOver;
}

void World::adaptPlayerVelocity(){
	sf::Vector2f velocity = mPlayerAircraft->getVelocity();
	if (velocity.x != 0 && velocity.y != 0)
		mPlayerAircraft->setVelocity(velocity / std::sqrt(2.f));
}

void World::adaptPlayerPosition(){
	sf::FloatRect viewBounds(mWorldView.getCenter() - mWorldView.getSize() / 2.f, mWorldView.getSize());
	const float borderDistance = 40.f;

	sf::Vector2f position = mPlayerAircraft->getPosition();
	position.x = std::max(position.x, viewBounds.left + borderDistance);
	position.x = std::min(position.x, viewBounds.left + viewBounds.width - borderDistance);
	position.y = std::max(position.y, viewBounds.top + borderDistance);
	position.y = std::min(position.y, viewBounds.top + viewBounds.height - borderDistance);

	mPlayerAircraft->setPosition(position);
}

bool World::hasActivePlayer() const {
	return !mPlayerAircraft->isDestroyed();
}

bool World::hasPlayerReachedEnd() const{
	return !mWorldBounds.contains(mPlayerAircraft->getPosition());
}

void World::updateSounds(){
	mSounds.setListenerPosition(mPlayerAircraft->getWorldPosition());
	mSounds.removeStoppedSounds();
}