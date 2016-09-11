﻿//
// ArtistPage.xaml.h
// Declaration of the ArtistPage class
//

#pragma once
#include "WrapPanel.h"
#include "TrackItemVM.h"
#include "ArtistItemVM.h"
#include "GroupedAlbums.h"
#include "ArtistPage.g.h"
#include "TracksPlaybackStateManager.h"
#include "IPageWithPreservedState.h"

namespace Tidal
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class ArtistPage sealed : public IPageWithPreservedState
	{
	private:
		std::int64_t _artistId;
		Microsoft::Graphics::Canvas::CanvasBitmap^ _albumBmp;
		concurrency::cancellation_token_source _cts;
		std::shared_ptr<TracksPlaybackStateManager> _tracksPlaybackManager;
		Platform::Collections::Vector<TrackItemVM^>^ _popularTracks;
	public:
		ArtistPage();
		// Inherited via IPageWithPreservedState
		virtual Platform::Object ^ GetStateToPreserve();
	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override {
			LoadAsync(e);
		}
		virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override {
			_cts.cancel();
			_cts = concurrency::cancellation_token_source();
		}
	private:

		concurrency::task<void> LoadAsync(Windows::UI::Xaml::Navigation::NavigationEventArgs^ args);
		concurrency::task<void> loadImageAsync(Platform::String^ url);
		void OnDrawHeaderImage(Microsoft::Graphics::Canvas::UI::Xaml::ICanvasAnimatedControl^ sender, Microsoft::Graphics::Canvas::UI::Xaml::CanvasAnimatedDrawEventArgs^ args);
		
		void OnAlbumClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		void OnVideoClicked(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		void OnSimilarArtistClicked(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		void OnAddFavoriteClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnRemoveFavoriteClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnPlayPopularTracks(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnRadioClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

	};
}
