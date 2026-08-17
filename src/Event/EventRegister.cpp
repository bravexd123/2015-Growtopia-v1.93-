#include <Event/EventPool.hpp>

#include <Event/TextEvents/RequestedName.hpp>
#include <Event/TextEvents/Action.hpp>

#include <Event/TextEvents/Actions/OnChatInput.hpp>
#include <Event/TextEvents/Actions/OnDialogReturn.hpp>
#include <Event/TextEvents/Actions/OnEnterGame.hpp>
#include <Event/TextEvents/Actions/OnFriends.hpp>
#include <Event/TextEvents/Actions/OnGrowId.hpp>
#include <Event/TextEvents/Actions/OnJoinRequest.hpp>
#include <Event/TextEvents/Actions/OnRefreshItemData.hpp>
#include <Event/TextEvents/Actions/OnQuit.hpp>
#include <Event/TextEvents/Actions/OnRespawn.hpp>
#include <Event/TextEvents/Actions/OnWrenchPlayer.hpp>
#include <Event/TextEvents/Actions/OnSetSkin.hpp>
#include <Event/TextEvents/Actions/OnStore.hpp>
#include <Event/TextEvents/Actions/OnTrade.hpp>
#include <Event/TextEvents/Actions/OnTrashDropInfo.hpp>

#include <Event/Dialogs/AccountDialog.hpp>
#include <Event/Dialogs/LockDialog.hpp>
#include <Event/Dialogs/TileDialog.hpp>
#include <Event/Dialogs/ConsumableDialogs.hpp>
#include <Event/Dialogs/TrashDropDialogs.hpp>
