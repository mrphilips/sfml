#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "Component.hpp"
#include <functional>
#include "../util/ResourceHolder.hpp"
#include "../util/ResourceIdentifier.hpp"

namespace GUI{
	class Button : public Component{

	public:
		typedef std::shared_ptr<Button> Ptr;
		typedef std::function<void()> Callback;

		enum Type{
			Normal,
			Selected,
			Pressed,
			TypeCount
		};

	public:
		Button(const FontHolder& fonts, const TextureHolder& textures);

		void setCallBack(Callback callback);
		void setText(const std::string& text);
		void setToggle(bool flag);

		virtual bool isSelectable() const;
		virtual void select();
		virtual void deselect();

		virtual void activate();
		virtual void deactivate();

		virtual void handleEvent(const sf::Event& event);

		sf::Vector2u getTextureSize() const;

	private:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
		void changeTexture(Type button);

	private:
		Callback mCallback;
		sf::Sprite mSprite;
		sf::Text mText;
		bool mIsToggle;

	};
}

#endif