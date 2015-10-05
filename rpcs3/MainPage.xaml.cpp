//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

#include "Emu/GameInfo.h"
#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsDir.h"
#include "Emu/FS/vfsFile.h"

using namespace rpcs3_uap;

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

GameInfo CurGameInfo;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
}


void rpcs3_uap::MainPage::gameList_Loading(Windows::UI::Xaml::FrameworkElement^ sender, Platform::Object^ args)
{
	for (const auto info : vfsDir(""))
	{
		if (info->flags & DirEntry_TypeDir)
		{
			TextBlock ^gameName = ref new TextBlock();
			gameName->Text = ref new String(std::wstring(info->name.begin(), info->name.end()).c_str());
			gameList->Items->Append(gameName);
		}
	}
}
