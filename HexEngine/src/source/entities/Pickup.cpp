#include "../../headers/entities/Pickup.hpp"

namespace{
	const std::vector<PickupData> Table = initializePickUpData();
}

Pickup::Pickup(Type type, const TextureHolder& textures):
Entity(1),
mType(type),
mSprite(textures.get(Table[type].texture))
{
}

void Pickup::apply(Aircraft& player) const{
	Table[mType].action(player);
}

void Pickup::drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const{
	target.draw(mSprite, states);
}

sf::FloatRect Pickup::getBoundingRect() const{
	return getWorldTransform().transformRect(mSprite.getGlobalBounds());
}

unsigned int Pickup::getCategory() const {
	return Category::Pickup;
}