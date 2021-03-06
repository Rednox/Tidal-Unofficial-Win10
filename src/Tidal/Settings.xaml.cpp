﻿//
// Settings.xaml.cpp
// Implementation of the Settings class
//

#include "pch.h"
#include "Settings.xaml.h"
#include <localdata/StorageStatistics.h>
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

Settings::Settings()
{
	InitializeComponent();
}

String^ bytesToString(std::int64_t bytes) {
	if (bytes < 1024) {
		return bytes.ToString() + L" B";
	}
	if (bytes < 1024 * 1024) {
		return (bytes / 1024.0).ToString() + L" KB";
	}
	if (bytes < 1024 * 1024 * 1024) {
		return (bytes / (1024 * 1024.0)).ToString() + L" MB";
	}
	return (bytes / (1024 * 1024 * 1024.0)).ToString() + L" GB";
}


concurrency::task<void> Tidal::Settings::loadStorageStatisticsAsync()
{
	auto stats = co_await localdata::getStorageStatisticsAsync(concurrency::cancellation_token::none());
	cacheTotal->Text = bytesToString(stats->totalCacheSize);
	cache8Days->Text = bytesToString(stats->cachedNotPlayedFor8DaysSize);
	importTotal->Text = bytesToString(stats->totalImportSize);
	import8Days->Text = bytesToString(stats->importNotPlayedFor8DaysSize);
}

void Tidal::Settings::OnPageLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto settingsValues = Windows::Storage::ApplicationData::Current->LocalSettings->Values;
	auto streamingQuality = dynamic_cast<Platform::String^>(settingsValues->Lookup(L"streamingQuality"));
	if (!streamingQuality) {
		streamingQuality = L"HIGH";
		settingsValues->Insert(L"streamingQuality", streamingQuality);
	}

	auto importQuality = dynamic_cast<Platform::String^>(settingsValues->Lookup(L"importQuality"));
	if (!importQuality) {
		importQuality = L"LOSSLESS";
		settingsValues->Insert(L"importQuality", importQuality);
	}

	if (streamingQuality == L"LOW") {
		streamLow->IsChecked = true;
	}
	else if (streamingQuality == L"HIGH") {
		streamHigh->IsChecked = true;
	}
	else if (streamingQuality == L"LOSSLESS") {
		streamLossless->IsChecked = true;
	}



	if (importQuality == L"LOW") {
		importLow->IsChecked = true;
	}
	else if (importQuality == L"HIGH") {
		importHigh->IsChecked = true;
	}
	else if (importQuality == L"LOSSLESS") {
		importLossless->IsChecked = true;
	}
	loadStorageStatisticsAsync();
}


void Tidal::Settings::OnStreamingQualityChecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto settingsValues = Windows::Storage::ApplicationData::Current->LocalSettings->Values;
	auto fe = dynamic_cast<FrameworkElement^>(sender);
	if (fe) {
		auto quality = dynamic_cast<String^>(fe->Tag);
		if (quality) {

			settingsValues->Insert(L"streamingQuality", quality);
		}
	}

}


void Tidal::Settings::OnImportQualityChecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto settingsValues = Windows::Storage::ApplicationData::Current->LocalSettings->Values;
	auto fe = dynamic_cast<FrameworkElement^>(sender);
	if (fe) {
		auto quality = dynamic_cast<String^>(fe->Tag);
		if (quality) {

			settingsValues->Insert(L"importQuality", quality);
		}
	}
}


void Tidal::Settings::OnClearCache_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	storageProgressBar->IsIndeterminate = true;
	concurrency::create_task([]() {
		return localdata::clearCacheAndGetStorageStatisticsAsync();
	}).then([this](std::shared_ptr<localdata::storage_statistics> stats) {
		cacheTotal->Text = bytesToString(stats->totalCacheSize);
		cache8Days->Text = bytesToString(stats->cachedNotPlayedFor8DaysSize);
		importTotal->Text = bytesToString(stats->totalImportSize);
		import8Days->Text = bytesToString(stats->importNotPlayedFor8DaysSize);
		storageProgressBar->IsIndeterminate = false;
	}, concurrency::task_continuation_context::get_current_winrt_context());

}


void Tidal::Settings::OnClearImports_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	storageProgressBar->IsIndeterminate = true;
	concurrency::create_task([]() {
		return localdata::clearImportsAndGetStorageStatisticsAsync();
	}).then([this](std::shared_ptr<localdata::storage_statistics> stats) {
		cacheTotal->Text = bytesToString(stats->totalCacheSize);
		cache8Days->Text = bytesToString(stats->cachedNotPlayedFor8DaysSize);
		importTotal->Text = bytesToString(stats->totalImportSize);
		import8Days->Text = bytesToString(stats->importNotPlayedFor8DaysSize);
		storageProgressBar->IsIndeterminate = false;
	}, concurrency::task_continuation_context::get_current_winrt_context());
}


void Tidal::Settings::OnRemoveOldCacheTracks_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	storageProgressBar->IsIndeterminate = true;
	concurrency::create_task([]() {
		return localdata::clearCacheNotPlayedFor8DaysAngGetStorageStatisticsAsync();
	}).then([this](std::shared_ptr<localdata::storage_statistics> stats) {
		cacheTotal->Text = bytesToString(stats->totalCacheSize);
		cache8Days->Text = bytesToString(stats->cachedNotPlayedFor8DaysSize);
		importTotal->Text = bytesToString(stats->totalImportSize);
		import8Days->Text = bytesToString(stats->importNotPlayedFor8DaysSize);
		storageProgressBar->IsIndeterminate = false;
	}, concurrency::task_continuation_context::get_current_winrt_context());
}


void Tidal::Settings::OnRemoveOldImportedTracks_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	storageProgressBar->IsIndeterminate = true;
	concurrency::create_task([]() {
		return localdata::clearImportsNotPlayedFor8DaysAngGetStorageStatisticsAsync();
	}).then([this](std::shared_ptr<localdata::storage_statistics> stats) {
		cacheTotal->Text = bytesToString(stats->totalCacheSize);
		cache8Days->Text = bytesToString(stats->cachedNotPlayedFor8DaysSize);
		importTotal->Text = bytesToString(stats->totalImportSize);
		import8Days->Text = bytesToString(stats->importNotPlayedFor8DaysSize);
		storageProgressBar->IsIndeterminate = false;
	}, concurrency::task_continuation_context::get_current_winrt_context());
}
