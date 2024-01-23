#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <windows.h>
#include <mutex>
#include <condition_variable>
#include <nlohmann/json.hpp>
#include "cpr/cpr.h"
#include <tchar.h>
#include <fstream>


std::atomic<bool> metronomeOn(false);
std::mutex metronomeMutex;
std::condition_variable metronomeCV;

void ToggleMetronome()
{
	std::unique_lock<std::mutex> lock(metronomeMutex);
	metronomeOn = !metronomeOn;
	metronomeCV.notify_all();
}

std::atomic<float> bpm(120.0f);

void refreshBPM()
{
	for (;;)
	{
		{
			std::unique_lock<std::mutex> lock(metronomeMutex);
			metronomeCV.wait(lock, [] { return metronomeOn.load(); });
		}

		Sleep(1000);

		cpr::Response r = cpr::Get(cpr::Url{ "https://127.0.0.1:2999/liveclientdata/activeplayer" }, cpr::VerifySsl{ false });

		if (!(r.status_code == 200))
		{
			continue;
		}

		using json = nlohmann::json;

		try
		{
			json jsonData = json::parse(r.text);

			if (jsonData.contains("championStats") && jsonData["championStats"].contains("attackSpeed")) {

				double attackSpeed = jsonData["championStats"]["attackSpeed"];

				bpm = attackSpeed * 60.0F;
			}
			else {
				std::cout << "attackSpeed key missing in JSON data." << std::endl;
			}
		}
		catch (json::exception& e) {
			std::cerr << "JSON parsing error: " << e.what() << std::endl;
		}
	}

}


void Metronome(sf::Sound& metronomeSound, sf::RenderWindow& window)
{
	float currentBPM = bpm.load(); 

	while (window.isOpen())
	{
		{
			std::unique_lock<std::mutex> lock(metronomeMutex);
			metronomeCV.wait(lock, [] { return metronomeOn.load(); }); 
		}
		if (currentBPM > 0.0f)
		{
			
			float interval = 60.0f / currentBPM;

			metronomeSound.play();

			Sleep(static_cast<DWORD>(interval * 1000));
		}
		else
		{
			Sleep(100);
		}

		currentBPM = bpm.load();
	}
}

BOOL IsElevated() {
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) {
		CloseHandle(hToken);
	}
	return fRet;
}

void leagueOfLegendsGameStatusUpdater(std::string& text)
{
	while (true)
	{
		Sleep(10000);
		cpr::Response r = cpr::Get(cpr::Url{ "https://127.0.0.1:2999/liveclientdata/activeplayer" }, cpr::VerifySsl{ false });
		if (!(r.status_code == 200))
		{
			text = "League of Legends match not started";
		}
		else
		{
			text = "League of Legends match started and everything works";
		}
	}
}


std::string GetCurrentDir()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");

	return std::string(buffer).substr(0, pos);
}

void addSound()
{

	CoInitialize(NULL);

	
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	/*ofn.lpstrFilter = _T("All Files\0*.*\0");*/
	/*ofn.lpstrInitialDir = L".\\sounds";*/
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	
	if (GetOpenFileName(&ofn)) {
		MessageBox(NULL, szFile, _T("Selected File"), MB_OK);
	}

	CoUninitialize();

	namespace fs = std::filesystem;
	fs::path sourceFile = szFile;
	fs::path targetParent = GetCurrentDir() + "\\sounds";
	std::cout << targetParent << std::endl;
	auto target = targetParent / sourceFile.filename();

	try
	{
		fs::create_directories(targetParent); 
		fs::copy_file(sourceFile, target, fs::copy_options::overwrite_existing);
	}
	catch (std::exception& e) 
	{
		std::cout << e.what();
	}
}

void setSound(sf::SoundBuffer& soundBuffer)
{
	// Initialize the COM library
	CoInitialize(NULL);

	// Initialize the OPENFILENAME structure
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	/*ofn.lpstrFilter = _T("All Files\0*.*\0");*/
	std::string currentDirStr = GetCurrentDir();
	std::wstring initialDir(currentDirStr.begin(), currentDirStr.end());
	initialDir = initialDir + L"\\sounds";
	ofn.lpstrInitialDir = initialDir.c_str();
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)) {
		MessageBox(NULL, szFile, _T("Selected File"), MB_OK);
	}

	// Release COM
	CoUninitialize();

	fs::path sourceFile = szFile;
	
	soundBuffer.loadFromFile(sourceFile.string());

	nlohmann::json jsonSettings;
	jsonSettings["file_name"] = sourceFile.filename();

	std::ofstream file(GetCurrentDir() + "\\settings.json");
	if (!file.is_open()) {
		std::cerr << "couldn't open file to save" << std::endl;
	}

	file << jsonSettings.dump(4);

	file.close();

}

int main()
{

	namespace fs = std::filesystem;

	fs::path currentDir = fs::current_path();
	std::cout << currentDir;

	fs::path soundsDir = currentDir / "sounds";

	if (!fs::is_directory(soundsDir)) {
		if (fs::create_directory(soundsDir)) {
			std::cout << "A 'sounds' folder has been created in the current directory." << std::endl;
		}
		else {
			std::cerr << "Could not create folder 'sounds'." << std::endl;
			return 1;
		}
	}

	sf::RenderWindow window(sf::VideoMode(1280, 720), "Metronom w SFML");
	sf::Event event;

	sf::Font font;

	nlohmann::json jsonSettings;
	std::ifstream file(GetCurrentDir() + "\\settings.json");
	if (!file.is_open()) {
		jsonSettings["file_name"] = "sound.ogg";
	}
	else {
		try {
			file >> jsonSettings;
			file.close();
		}
		catch (const nlohmann::json::exception& e) {
			std::cerr << "Error while parsing json file: " << e.what() << std::endl;
			jsonSettings["file_name"] = "sound.ogg";
		}
	}

	std::string fileName = jsonSettings["file_name"];

	sf::SoundBuffer metronomeSoundBuffer;
	if (!metronomeSoundBuffer.loadFromFile(GetCurrentDir() + "\\sounds\\" + fileName)) {
		std::cerr << "Failed to load metronome sound." << std::endl;
		if (!metronomeSoundBuffer.loadFromFile(GetCurrentDir() + "\\sounds\\sound.ogg")) {
			std::cerr << "Failed to load default metronome sound." << std::endl;
			return 2137;
		}
	}
	
	font.loadFromFile("\\JetBrainsMono-Regular.ttf");

	//"title"
	sf::Text text;
	text.setFont(font);
	text.setString("League of Legends BPM");
	text.setCharacterSize(54);
	text.setFillColor(sf::Color::White); 
	text.setPosition(330, 90); 

	 // tip rectangle
	sf::RectangleShape tipRectangle(sf::Vector2f(450, 50)); 
	tipRectangle.setPosition(350, 250); 
	tipRectangle.setFillColor(sf::Color(100, 100, 100)); 

	sf::Text tipText("Toogle metronome - M button", font, 24); 
	tipText.setFillColor(sf::Color::White); 
	tipText.setPosition(390, 260);

	//isAdminRectangle

	sf::RectangleShape isAdminRectangle(sf::Vector2f(400, 50));
	isAdminRectangle.setPosition(10, window.getSize().y - 60);

	sf::Text isAdmintext;
	isAdmintext.setFont(font);
	isAdmintext.setCharacterSize(20);
	isAdmintext.setPosition(20, window.getSize().y - 50);

	
	bool isAdmin = IsElevated();

	if (isAdmin) {
		isAdminRectangle.setFillColor(sf::Color::Green);
		isAdmintext.setString("Run as Administrator");
		isAdmintext.setFillColor(sf::Color::White);
	}
	else {
		isAdminRectangle.setFillColor(sf::Color::Red);
		isAdmintext.setString("Not Run as Administrator");
		isAdmintext.setFillColor(sf::Color::White);
	}

	//league of legends status

	sf::RectangleShape league_of_legends_game_status(sf::Vector2f(800, 50));
	league_of_legends_game_status.setPosition(420, window.getSize().y - 60);
	league_of_legends_game_status.setFillColor(sf::Color(100, 100, 100)); 


	std::string league_of_legends_game_status_string = "checking..";
	sf::Text league_of_legends_game_status_text("Unknow", font, 24);
	league_of_legends_game_status_text.setPosition(430, window.getSize().y - 50);

	//sound add
	sf::RectangleShape button(sf::Vector2f(450, 50));
	button.setPosition(350, 350);
	button.setFillColor(sf::Color(100, 100, 100));
	sf::Text buttonText("Add Sound", font, 24);
	buttonText.setPosition(390, 360);

	//set sound
	sf::RectangleShape setSoundRectangle(sf::Vector2f(450, 50));
	setSoundRectangle.setPosition(350, 450);
	setSoundRectangle.setFillColor(sf::Color(100, 100, 100));
	sf::Text setSoundText("Set sound", font, 24);
	setSoundText.setPosition(390, 460);

	window.setFramerateLimit(60);

	sf::Sound metronomeSound;
	metronomeSound.setBuffer(metronomeSoundBuffer);

	
	std::thread metronomeThread(Metronome, std::ref(metronomeSound), std::ref(window));
	std::thread testThread(refreshBPM);
	std::thread statusUpdater(leagueOfLegendsGameStatusUpdater, std::ref(league_of_legends_game_status_string));

	while (window.isOpen())
	{
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
			}
			if (event.type == sf::Event::MouseButtonReleased) {
				if (event.mouseButton.button == sf::Mouse::Left) {
					sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
					if (button.getGlobalBounds().contains(mousePos)) {
						addSound();
					}
					if (setSoundRectangle.getGlobalBounds().contains(mousePos))
					{
						setSound(metronomeSoundBuffer);
					}
				}
			}

		}
		
		if (GetAsyncKeyState('M') & 0x8000)
		{
			ToggleMetronome();
			Sleep(200);
		}

		window.clear(sf::Color(49, 61, 90));
		window.draw(text);
		window.draw(tipRectangle);
		window.draw(tipText);
		window.draw(isAdminRectangle);
		window.draw(isAdmintext);
		window.draw(league_of_legends_game_status);
		league_of_legends_game_status_text.setString(league_of_legends_game_status_string);
		window.draw(league_of_legends_game_status_text);
		window.draw(button);
		window.draw(buttonText);
		window.draw(setSoundRectangle);
		window.draw(setSoundText);
		window.display();
	}

	metronomeThread.join();
	testThread.join();
	statusUpdater.join();
	

	return 0;
}

