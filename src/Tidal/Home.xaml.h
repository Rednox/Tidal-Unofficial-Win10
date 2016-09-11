﻿//
// Home.xaml.h
// Declaration of the Home class
//

#pragma once
#include "MetricsHelper.h"
#include "BindableSizeHelper.h"
#include "PromotionItemVM.h"
#include "VideoItemVM.h"
#include "AlbumResumeItemVM.h"
#include "PlaylistResumeItemVM.h"
#include "TrackItemVM.h"
#include "FactorConverter.h"
#include "Home.g.h"
#include "SublistItemVM.h"
#include "TracksPlaybackStateManager.h"
#include "IPageWithPreservedState.h"
#include "Shell.xaml.h"

namespace Tidal
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class Home sealed :  public IPageWithPreservedState
	{
	private:
		concurrency::cancellation_token_source _cts;
		std::shared_ptr<TracksPlaybackStateManager> _tracksPlaybackManager;
		bool _isRestoringState = false;
	public:
		Home();
		virtual	Platform::Object^ GetStateToPreserve() ;
	protected:

		virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override {
			_cts.cancel();
			_cts = concurrency::cancellation_token_source();
		}
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
	private:
		void OnPromotionClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		void OnVideoClicked(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		concurrency::task<void> LoadTracksAsync();
		void OnAlbumClicked(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		void OnHomeLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnPlaylistClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		void OnTrackClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		void OnVideoFilterChanged(Platform::Object^ sender, Tidal::SublistItemVM^ e);
		void OnPlaylistFilterChanged(Platform::Object^ sender, Tidal::SublistItemVM^ e);
		void OnAlbumFilterChanged(Platform::Object^ sender, Tidal::SublistItemVM^ e);
		void OnTrackFilterChanged(Platform::Object^ sender, Tidal::SublistItemVM^ e);
	};
}
