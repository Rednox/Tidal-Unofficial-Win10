#include "pch.h"
#include "FavoritesService.h"
#include <Api/FavoriteQueries.h>
#include <Api/GetPlaylistQuery.h>
#include "AuthenticationService.h"
#include <Api/GetAlbumResumeQuery.h>
#include <Api/GetArtistInfoQuery.h>
#include <Api/GetTrackInfoQuery.h>
#include <tools/AsyncHelpers.h>
#include <tools/TimeUtils.h>
#include "Mediators.h"

struct refresh_failed {

};
std::unique_ptr<FavoritesService> g_favSvc;
std::once_flag g_favOnce;
FavoritesService & getFavoriteService()
{
	std::call_once(g_favOnce, []() {g_favSvc.reset(new FavoritesService()); });
	return *g_favSvc;
}

concurrency::task<void> FavoritesService::doRefreshAsync(concurrency::cancellation_token cancelToken)
{
	auto& authState = getAuthenticationService().authenticationState();
	bool refreshFailed = false;
	if (!authState.isAuthenticated()) {
		{
			std::lock_guard<std::recursive_mutex> lg(_mutex);
			_albumIds.clear();
			_artistIds.clear();
			_trackIds.clear();
			_playlistIds.clear();

			_albums->Clear();
			_artists->Clear();
			_playlists->Clear();
			_myPlaylists->Clear();
			_tracks->Clear();
		}
		co_await concurrency::task_from_result();
	}
	else {
		auto albumsTask = api::GetFavoriteAlbumsQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode())->executeAsync(cancelToken);
		auto artistsTask = api::GetFavoriteArtistsQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode())->executeAsync(cancelToken);
		auto tracksTask = api::GetFavoriteTracksQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode())->executeAsync(cancelToken);
		auto playlistsTask = api::GetFavoritePlaylistsQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode())->executeAsync(cancelToken);
		auto myPlaylistsTask = api::GetMyPlaylistsQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode())->executeAsync(cancelToken);

		try {
			auto albums = co_await albumsTask;
			std::sort(albums->items.begin(), albums->items.end(), [](const auto& lhs, const auto& rhs) {
				return lhs.created > rhs.created;
			});
			{
				std::lock_guard<std::recursive_mutex> lg(_mutex);
				_albumIds.clear();
				_albums->Clear();
				for (auto&& item : albums->items) {
					_albumIds.insert(item.item.id);
					_albums->Append(ref new Tidal::AlbumResumeItemVM(item.item));
				}
			}
		}
		catch (concurrency::task_canceled&) {}
		catch (...) {
			refreshFailed = true;
		}

		try {
			auto artists = co_await artistsTask;

			std::sort(artists->items.begin(), artists->items.end(), [](const auto& lhs, const auto& rhs) {
				return lhs.created > rhs.created;
			});
			{
				std::lock_guard<std::recursive_mutex> lg(_mutex);
				_artistIds.clear();
				_artists->Clear();
				for (auto&& item : artists->items) {
					_artistIds.insert(item.item.id);
					_artists->Append(ref new Tidal::ArtistItemVM(item.item));
				}
			}
		}
		catch (concurrency::task_canceled&) {}
		catch (...) {
			refreshFailed = true;
		}
		try {
			auto tracks = co_await tracksTask;

			std::sort(tracks->items.begin(), tracks->items.end(), [](const auto& lhs, const auto& rhs) {
				return lhs.created > rhs.created;
			});
			{
				std::lock_guard<std::recursive_mutex> lg(_mutex);
				_trackIds.clear();
				_tracks->Clear();
				for (auto&& item : tracks->items) {
					_trackIds.insert(item.item.id);
					auto ti = ref new Tidal::TrackItemVM(item.item);
					_tracks->Append(ti);
					ti->AttachTo(_tracks);
				}
			}

		}
		catch (concurrency::task_canceled&) {}
		catch (...) {
			refreshFailed = true;
		}


		try {
			auto playlists = co_await playlistsTask;

			std::sort(playlists->items.begin(), playlists->items.end(), [](const auto& lhs, const auto& rhs) {
				return lhs.created > rhs.created;
			});
			{
				std::lock_guard<std::recursive_mutex> lg(_mutex);
				_playlistIds.clear();
				_playlists->Clear();
				for (auto&& item : playlists->items) {
					_playlistIds.insert(item.item.uuid);
					_playlists->Append(ref new Tidal::PlaylistResumeItemVM(item.item));
				}
			}
		}
		catch (concurrency::task_canceled&) {}
		catch (...) {
			refreshFailed = true;
		}

		try {
			auto myPlaylists = co_await myPlaylistsTask;
			{
				std::lock_guard<std::recursive_mutex> lg(_mutex);
				_myPlaylists->Clear();
				for (auto&& item : myPlaylists->items) {
					_myPlaylists->Append(ref new Tidal::PlaylistResumeItemVM(item));
				}
			}
		}
		catch (concurrency::task_canceled&) {}
		catch (...) {
			refreshFailed = true;
		}

		if (cancelToken.is_canceled()) {
			concurrency::cancel_current_task();
		}
		else if(refreshFailed) {
			throw refresh_failed();
		}
	}
	getFavoritesRefreshedMediator().raise(true);
}

concurrency::task<void> FavoritesService::refreshAsync(concurrency::cancellation_token cancelToken)
{
	return tools::async::retryWithDelays([this, cancelToken]() {return doRefreshAsync(cancelToken); }, tools::time::ToWindowsTimeSpan(std::chrono::seconds(5)), cancelToken);
}

bool FavoritesService::isAlbumFavorite(std::int64_t id)
{
	std::lock_guard<std::recursive_mutex> lg(_mutex);
	return _albumIds.find(id) != _albumIds.end();
}

bool FavoritesService::isArtistFavorite(std::int64_t id)
{
	std::lock_guard<std::recursive_mutex> lg(_mutex);
	return _artistIds.find(id) != _artistIds.end();
}

bool FavoritesService::isTrackFavorite(std::int64_t id)
{
	std::lock_guard<std::recursive_mutex> lg(_mutex);
	return _trackIds.find(id) != _trackIds.end();
}

bool FavoritesService::isPlaylistFavorite(const std::wstring & id)
{
	std::lock_guard<std::recursive_mutex> lg(_mutex);
	return _playlistIds.find(id) != _playlistIds.end();
}

concurrency::task<void> FavoritesService::addPlaylistAsync(const std::wstring & idRef)
{
	auto id = idRef;
	auto& authState = getAuthenticationService().authenticationState();
	if (!authState.isAuthenticated()) {
		co_await concurrency::task_from_result();
	}
	else {
		co_await api::AddFavoritePlaylistQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode(), tools::strings::toWindowsString(id))->executeAsync(concurrency::cancellation_token::none());
		auto playlist = co_await api::GetPlaylistQuery(id, authState.sessionId(), authState.countryCode()).executeAsync(concurrency::cancellation_token::none());

		_playlistIds.insert(playlist->uuid);
		_playlists->InsertAt(0, ref new Tidal::PlaylistResumeItemVM(*playlist));
	}
}

concurrency::task<void> FavoritesService::removePlaylistAsync(const std::wstring & idRef)
{
	auto id = idRef;
	auto& authState = getAuthenticationService().authenticationState();
	if (!authState.isAuthenticated()) {
		co_await concurrency::task_from_result();
	}
	else {
		co_await api::RemoveFavoritePlaylistQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode(), tools::strings::toWindowsString(id))->executeAsync(concurrency::cancellation_token::none());

		_playlistIds.erase(id);
		auto found = std::find_if(begin(_playlists), end(_playlists), [uuid = tools::strings::toWindowsString(id)](Tidal::PlaylistResumeItemVM^ item) {
			return item->Uuid == uuid;
		});
		if (found != end(_playlists)) {
			_playlists->RemoveAt(found - begin(_playlists));
		}

	}
}

concurrency::task<void> FavoritesService::addAlbumAsync(std::int64_t id)
{
	auto& authState = getAuthenticationService().authenticationState();
	if (!authState.isAuthenticated()) {
		co_await concurrency::task_from_result();
	}
	else {
		co_await api::AddFavoriteAlbumQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode(), id)->executeAsync(concurrency::cancellation_token::none());
		auto album = co_await api::GetAlbumResumeQuery(id, authState.sessionId(), authState.countryCode()).executeAsync(concurrency::cancellation_token::none());

		_albumIds.insert(id);
		_albums->InsertAt(0, ref new Tidal::AlbumResumeItemVM(*album));
	}
}

concurrency::task<void> FavoritesService::removeAlbumAsync(std::int64_t id)
{
	auto& authState = getAuthenticationService().authenticationState();
	if (!authState.isAuthenticated()) {
		co_await concurrency::task_from_result();
	}
	else {
		co_await api::RemoveFavoriteAlbumQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode(), id)->executeAsync(concurrency::cancellation_token::none());

		_albumIds.erase(id);
		auto found = std::find_if(begin(_albums), end(_albums), [id](Tidal::AlbumResumeItemVM^ item) {
			return item->Id == id;
		});
		if (found != end(_albums)) {
			_albums->RemoveAt(found - begin(_albums));
		}

	}
}

concurrency::task<void> FavoritesService::addArtistAsync(std::int64_t id)
{
	auto& authState = getAuthenticationService().authenticationState();
	if (!authState.isAuthenticated()) {
		co_await concurrency::task_from_result();
	}
	else {
		co_await api::AddFavoriteArtistQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode(), id)->executeAsync(concurrency::cancellation_token::none());
		auto item = co_await api::GetArtistInfoQuery(id, authState.sessionId(), authState.countryCode()).executeAsync(concurrency::cancellation_token::none());

		_artistIds.insert(id);
		_artists->InsertAt(0, ref new Tidal::ArtistItemVM(*item));
	}
}

concurrency::task<void> FavoritesService::removeArtistAsync(std::int64_t id)
{
	auto& authState = getAuthenticationService().authenticationState();
	if (!authState.isAuthenticated()) {
		co_await concurrency::task_from_result();
	}
	else {
		co_await api::RemoveFavoriteArtisttQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode(), id)->executeAsync(concurrency::cancellation_token::none());

		_artistIds.erase(id);
		auto found = std::find_if(begin(_artists), end(_artists), [id](Tidal::ArtistItemVM^ item) {
			return item->Id == id;
		});
		if (found != end(_artists)) {
			_artists->RemoveAt(found - begin(_artists));
		}

	}
}

concurrency::task<void> FavoritesService::addTrackAsync(std::int64_t id)
{
	auto& authState = getAuthenticationService().authenticationState();
	if (!authState.isAuthenticated()) {
		co_await concurrency::task_from_result();
	}
	else {
		co_await api::AddFavoriteTrackQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode(), id)->executeAsync(concurrency::cancellation_token::none());
		auto item = co_await api::GetTrackInfoQuery(id, authState.sessionId(), authState.countryCode()).executeAsync(concurrency::cancellation_token::none());

		_trackIds.insert(id);
		_tracks->InsertAt(0, ref new Tidal::TrackItemVM(*item));
	}
}

concurrency::task<void> FavoritesService::removeTrackAsync(std::int64_t id)
{
	auto& authState = getAuthenticationService().authenticationState();
	if (!authState.isAuthenticated()) {
		co_await concurrency::task_from_result();
	}
	else {
		co_await api::RemoveFavoriteTrackQuery::Make(authState.userId(), authState.sessionId(), authState.countryCode(), id)->executeAsync(concurrency::cancellation_token::none());

		_trackIds.erase(id);
		auto found = std::find_if(begin(_tracks), end(_tracks), [id](Tidal::TrackItemVM^ item) {
			return item->Id == id;
		});
		if (found != end(_tracks)) {
			_tracks->RemoveAt(found - begin(_tracks));
		}

	}
}
