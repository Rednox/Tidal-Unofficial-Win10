﻿//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "XboxOneHub.h"
#include "MainPage.xaml.h"
#include "Shell.xaml.h"
#include "AudioService.h"
#include "Mediators.h"
#include <localdata/db.h>
#include "FavoritesService.h"
#include "AuthenticationVisualStateTrigger.h"
#include "XboxUI/XboxShell.xaml.h"
#include <Environment.h>
using namespace Tidal;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
	InitializeComponent();
	if (env::isRunningOnXbox()) {
		RequiresPointerMode = Windows::UI::Xaml::ApplicationRequiresPointerMode::WhenRequested;
	}
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
	Resuming += ref new Windows::Foundation::EventHandler<Platform::Object ^>(this, &Tidal::App::OnResuming);
	EnteredBackground += ref new Windows::UI::Xaml::EnteredBackgroundEventHandler(this, &Tidal::App::OnEnteredBackground);
	LeavingBackground += ref new Windows::UI::Xaml::LeavingBackgroundEventHandler(this, &Tidal::App::OnLeavingBackground);
}

/// <summary>
/// Invoked when the application is launched normally by the end user.	Other entry points
/// will be used such as when the application is launched to open a specific file.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e)
{
	if (_smtcService) {
		return;
	}
	
	Windows::Graphics::Display::DisplayInformation::AutoRotationPreferences = Windows::Graphics::Display::DisplayOrientations::Landscape
		| Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped
		| Windows::Graphics::Display::DisplayOrientations::Portrait
		| Windows::Graphics::Display::DisplayOrientations::PortraitFlipped;
#if _DEBUG
	// Show graphics profiling information while debugging.
	if (IsDebuggerPresent())
	{
		// Display the current frame rate counters
		DebugSettings->EnableFrameRateCounter = true;
	}
#endif

	getFavoriteService().refreshAsync(concurrency::cancellation_token::none()).then([](concurrency::task<void>t) {
		try {
			t.get();
		}
		catch (...) {}
	});

	localdata::initializeDbAsync().then([this,e]() {
		_smtcService = std::make_unique<SmtcService>(Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher);
		getAudioService().wakeupDownloaderAsync(concurrency::cancellation_token::none());

		if (env::isRunningOnXbox()) {
			if (Window::Current->Content != nullptr) {
				return;
			}
			auto xbshell = ref new XboxShell();
			Window::Current->Content = xbshell;
			Window::Current->Activate();
			return;
		}

		auto rootFrame = dynamic_cast<Shell^>(Window::Current->Content);

		// Do not repeat app initialization when the Window already has content,
		// just ensure that the window is active
		if (rootFrame == nullptr)
		{
			// Create a Frame to act as the navigation context and associate it with
			// a SuspensionManager key
			rootFrame = ref new Shell(nullptr, _persistedState);


			if (e->PreviousExecutionState == ApplicationExecutionState::Terminated)
			{
				// TODO: Restore the saved session state only when appropriate, scheduling the
				// final launch steps after the restore is complete

			}


			// Place the frame in the current Window
			Window::Current->Content = rootFrame;
			// Ensure the current window is active
			Window::Current->Activate();
		}
		else
		{
			if (rootFrame->Content == nullptr)
			{
				// When the navigation stack isn't restored navigate to the first page,
				// configuring the new page by passing required information as a navigation
				// parameter
			}
			// Ensure the current window is active
			Window::Current->Activate();
			_backMouseToken = Window::Current->CoreWindow->PointerPressed += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow ^, Windows::UI::Core::PointerEventArgs ^>([](Windows::UI::Core::CoreWindow ^, Windows::UI::Core::PointerEventArgs ^ args) {
				if (args->CurrentPoint->Properties->IsXButton1Pressed) {
					auto frame = dynamic_cast<Shell^>(Window::Current->Content)->Frame;
					if (frame->CanGoBack) {
						frame->GoBack();
					}
				}
			});
		}
	}, concurrency::task_continuation_context::get_current_winrt_context());
}

void Tidal::App::OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs ^ e)
{
	auto kind = e->Kind;
}

/// <summary>
/// Invoked when application execution is being suspended.	Application state is saved
/// without knowing whether the application will be terminated or resumed with the contents
/// of memory still intact.
/// </summary>
/// <param name="sender">The source of the suspend request.</param>
/// <param name="e">Details about the suspend request.</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	(void)sender;	// Unused parameter
	(void)e;	// Unused parameter
	auto deferral = e->SuspendingOperation->GetDeferral();
	_smtcService.reset();
	getAudioService().onSuspending();
	getAppSuspendingMediator().raise(true).then([deferral]() {deferral->Complete(); });
}

/// <summary>
/// Invoked when Navigation to a certain page fails
/// </summary>
/// <param name="sender">The Frame which failed navigation</param>
/// <param name="e">Details about the navigation failure</param>
void App::OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e)
{
	throw ref new FailureException("Failed to load Page " + e->SourcePageType.Name);
}

void Tidal::App::OnResuming(Platform::Object ^sender, Platform::Object ^args)
{

	_smtcService = std::make_unique<SmtcService>(Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher);
	getFavoriteService().refreshAsync(concurrency::cancellation_token::none()).then([](concurrency::task<void>t) {
		try {
			t.get();
		}
		catch (...) {}
	});
	getAudioService().onResuming();
	getAppResumingMediator().raise(true);
}


void Tidal::App::OnEnteredBackground(Platform::Object^ sender, Windows::ApplicationModel::EnteredBackgroundEventArgs^ e)
{
	auto shell = dynamic_cast<Shell^>(Window::Current->Content);
	if (shell) {
		_persistedState = shell->SavePageStateForBackground();
		_navState = shell->Frame->GetNavigationState();
	}
	Window::Current->Content = nullptr;
}


void Tidal::App::OnLeavingBackground(Platform::Object^ sender, Windows::ApplicationModel::LeavingBackgroundEventArgs^ e)
{
	if (env::isRunningOnXbox()) {
		if (Window::Current->Content != nullptr) {
			return;
		}
		auto xbshell = ref new XboxShell();
		Window::Current->Content = xbshell;
		Window::Current->Activate();
	}
	else {
		auto shell = ref new Shell(_navState, _persistedState);
		Window::Current->Content = shell;
	}
}
