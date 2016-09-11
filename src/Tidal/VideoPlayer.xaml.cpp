﻿//
// VideoPlayer.xaml.cpp
// Implementation of the VideoPlayer class
//

#include "pch.h"
#include "VideoPlayer.xaml.h"
#include "AuthenticationService.h"
#include "Api/GetVideoUrlQuery.h"
#include "tools/StringUtils.h"
#include "AudioService.h"
#include <Environment.h>
#include "XboxUI\FocusHelper.h"
using namespace Tidal;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

concurrency::task<void> Tidal::VideoPlayer::launchVideo(Platform::String^ id)
{
	try {
		auto& authSvc = getAuthenticationService();
		if (!authSvc.authenticationState().isAuthenticated()) {
			auto dlg = ref new Windows::UI::Popups::MessageDialog(L"You are not authorized to watch this video", L"Access denied");
			dlg->ShowAsync();
		}
		else {
			api::GetVideoUrlQuery query(authSvc.authenticationState().sessionId(), authSvc.authenticationState().countryCode(), id);
			auto urlInfo = co_await query.executeAsync(concurrency::cancellation_token::none());
			me->Source = ref new Uri(tools::strings::toWindowsString(urlInfo->url));
			co_await getAudioService().pauseAsync();
		}
	}
	catch (...) {}
}

void Tidal::VideoPlayer::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{
	auto id = dynamic_cast<Platform::String^>(e->Parameter);
	launchVideo(id);
}

VideoPlayer::VideoPlayer()
{
	InitializeComponent();
	if (env::isRunningOnXbox()) {
		Margin = Thickness(48, 0, 48, 27);
	}
}


void Tidal::VideoPlayer::OnMediaFailed(Platform::Object^ sender, Windows::UI::Xaml::ExceptionRoutedEventArgs^ e)
{
	auto message = e->ErrorMessage;
	
}


void Tidal::VideoPlayer::OnLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
}
