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

std::atomic<bool> metronomeOn(true); // Zmienna przechowuj¹ca stan metronomu (w³¹czony/wy³¹czony).
std::mutex metronomeMutex;
std::condition_variable metronomeCV;

void ToggleMetronome()
{
    std::unique_lock<std::mutex> lock(metronomeMutex);
    metronomeOn = !metronomeOn; // Zmiana stanu metronomu po naciœniêciu 'm'.
    if (metronomeOn)
    {
        std::cout << "Metronome is ON" << std::endl;
    }
    else
    {
        std::cout << "Metronome is OFF" << std::endl;
    }
    metronomeCV.notify_one(); // Powiadom inny w¹tek, ¿e stan metronomu siê zmieni³.
}




std::atomic<float> bpm(120.0f); // U¿ywamy zmiennej atomowej do przechowywania BPM.
void test() {
   

    for (;;)
    {
        Sleep(1000);

        cpr::Response r = cpr::Get(cpr::Url{ "https://127.0.0.1:2999/liveclientdata/activeplayer" }, cpr::VerifySsl{ false });
        r.status_code;                  // 200
        r.header["content-type"];       // application/json; charset=utf-8
        r.text;                         // JSON text string
        /*std::cout << r.text << std::endl;
        std::cout << r.status_code << std::endl;
        std::cout << r.elapsed << std::endl;*/

        if (!(r.status_code == 200))
        {
            continue;
        }

        using json = nlohmann::json;

        try {


            // Konwertujemy JSON string na obiekt json
            json jsonData = json::parse(r.text);

            // Sprawdzamy, czy istnieje klucz "attackSpeed" w obiekcie "championStats"
            if (jsonData.contains("championStats") && jsonData["championStats"].contains("attackSpeed")) {
                // Pobieramy wartoœæ "attackSpeed" jako double
                double attackSpeed = jsonData["championStats"]["attackSpeed"];

                // Wyœwietlamy wartoœæ "attackSpeed"
                /*std::cout << "Attack Speed: " << attackSpeed << std::endl;*/
                bpm = attackSpeed * 60.0F;
            }
            else {
                std::cout << "Brak klucza 'attackSpeed' w danych JSON." << std::endl;
            }
        }
        catch (json::exception& e) {
            std::cerr << "B³¹d parsowania JSON: " << e.what() << std::endl;
        }
    }
    
}

//void Metronome(sf::Sound& metronomeSound, sf::RenderWindow& window)
//{
//    sf::Clock clock;
//    float elapsedTime = 0.0f;
//
//    while (window.isOpen())
//    {
//        
//            /*std::cout << "bpm = " << bpm << std::endl;*/
//            // Zliczanie czasu
//            elapsedTime += clock.restart().asSeconds();
//
//            // Odtwarzaj dŸwiêk metronomu i resetuj czas, gdy up³yn¹³ interwa³
//            if (elapsedTime >= 60.0f / bpm.load()) // U¿ycie load() do odczytu wartoœci BPM.
//            {
//                metronomeSound.play();
//                elapsedTime = 0.0f;
//            }
//        
//    }
//}

//void Metronome(sf::Sound& metronomeSound, sf::RenderWindow& window)
//{
//    sf::Clock clock;
//    float elapsedTime = 0.0f;
//    float currentBPM = bpm.load(); // Odczytaj wartoœæ BPM tylko raz
//
//    while (window.isOpen())
//    {
//        // Zliczanie czasu tylko wtedy, gdy metronom jest w³¹czony
//        if (currentBPM > 0.0f)
//        {
//            elapsedTime += clock.restart().asSeconds();
//
//            // Odtwarzaj dŸwiêk metronomu i resetuj czas, gdy up³yn¹³ interwa³
//            if (elapsedTime >= 60.0f / currentBPM)
//            {
//                metronomeSound.play();
//                elapsedTime = 0.0f;
//            }
//        }
//    }
//}

void Metronome(sf::Sound& metronomeSound, sf::RenderWindow& window)
{
    float currentBPM = bpm.load(); // Odczytaj wartoœæ BPM tylko raz

    while (window.isOpen())
    {
        {
            std::unique_lock<std::mutex> lock(metronomeMutex);
            metronomeCV.wait(lock, [] { return metronomeOn.load(); }); // Czekaj, a¿ metronom zostanie w³¹czony.
        }
        if (currentBPM > 0.0f)
        {
            // Oblicz interwa³ na podstawie BPM
            float interval = 60.0f / currentBPM;

            // Odtwarzaj dŸwiêk metronomu
            metronomeSound.play();

            // Uœpij w¹tek na interwa³
            Sleep(static_cast<DWORD>(interval * 1000)); // Przemnó¿ przez 1000, aby uzyskaæ milisekundy
        }
        else
        {
            // Jeœli metronom jest wy³¹czony, uœpij w¹tek na chwilê, aby zmniejszyæ obci¹¿enie
            Sleep(100);
        }

        // Ponownie odczytaj wartoœæ BPM na pocz¹tku ka¿dej iteracji
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

int main()
{

    std::cout<<IsElevated();

    sf::RenderWindow window(sf::VideoMode(1280, 720), "Metronom w SFML");
    sf::Event event;

    sf::Font font;
    //if (!font.loadFromFile("arial.ttf")) {
    //    return 1; // B³¹d ³adowania czcionki
    //}

    font.loadFromFile("\\JetBrainsMono-Regular.ttf");
    

    sf::Text text;
    text.setFont(font);
    text.setString("League of Legends BPM");
    text.setCharacterSize(54);
    text.setFillColor(sf::Color::White); // Kolor tekstu
    text.setPosition(330, 90); // Pozycja tekstu

     // Tworzenie przycisku
    sf::RectangleShape button(sf::Vector2f(400, 50)); // Rozmiar przycisku
    button.setPosition(350, 250); // Pozycja przycisku
    button.setFillColor(sf::Color(100, 100, 100)); // Kolor t³a przycisku

    sf::Text buttonText("Change metronome hotkey", font, 24); // Tekst na przycisku
    buttonText.setFillColor(sf::Color::White); // Kolor tekstu
    buttonText.setPosition(390, 260); // Pozycja tekstu

    sf::RectangleShape isAdminRectangle(sf::Vector2f(400, 50));
    isAdminRectangle.setPosition(10, window.getSize().y - 60); // Ustawienie w lewym dolnym rogu
  
    sf::Text isAdmintext;
    isAdmintext.setFont(font);
    isAdmintext.setCharacterSize(20);
    isAdmintext.setPosition(20, window.getSize().y - 50); // Ustawienie tekstu w prostok¹cie

    // SprawdŸ, czy program jest uruchomiony jako administrator
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

       // ################################
       // ||                            ||
       // ||League of Legends status bpm||
       // ||                            ||
       // ################################

    sf::RectangleShape league_of_legends_game_status(sf::Vector2f(800, 50));
    league_of_legends_game_status.setPosition(420, window.getSize().y - 60);
    league_of_legends_game_status.setFillColor(sf::Color(100, 100, 100)); // Kolor t³a przycisku


    std::string league_of_legends_game_status_string = "checking..";
    sf::Text league_of_legends_game_status_text("Unknow", font, 24);
    league_of_legends_game_status_text.setPosition(430, window.getSize().y - 50);
    


    // Inicjalizacja dŸwiêku metronomu
    sf::SoundBuffer metronomeSoundBuffer;
    if (!metronomeSoundBuffer.loadFromFile(".\\sound.ogg")) {
        std::cerr << "Failed to load metronome sound." << std::endl;
        return 1;
    }

    window.setFramerateLimit(60);

    sf::Sound metronomeSound;
    metronomeSound.setBuffer(metronomeSoundBuffer);

    // Rozpocznij w¹tek metronomu
    std::thread metronomeThread(Metronome, std::ref(metronomeSound), std::ref(window));
    std::thread testThread(test);
    std::thread statusUpdater(leagueOfLegendsGameStatusUpdater,std::ref(league_of_legends_game_status_string));

    while (window.isOpen())
    {
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
           /* if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::M)
            {
                ToggleMetronome();
            }*/
            // Obs³uga klikniêcia mysz¹
            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    if (button.getGlobalBounds().contains(mousePos)) {
                        // Obs³u¿ klikniêcie przycisku
                        // Tutaj mo¿esz dodaæ kod obs³ugi zdarzenia
                        std::cout << "Przycisk zosta³ klikniêty!" << std::endl;
                    }
                }
            }
        }
        /* SprawdŸ stan klawisza 'm' niezale¿nie od stanu okna*/
        if (GetAsyncKeyState('M') & 0x8000)
        {
            ToggleMetronome();
            Sleep(200); // OpóŸnienie, aby unikn¹æ wielokrotnego wykrywania naciœniêcia.
        }

        // Zwiêksz BPM o 10
        //if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        //    float currentBPM = bpm.load();
        //    bpm.store(currentBPM + 1.0f);
        //}
        //// Zmniejsz BPM o 10
        //else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        //    float currentBPM = bpm.load();
        //    bpm.store(currentBPM - 1.0f);
        //}

        

        window.clear(sf::Color(49, 61, 90));
        window.draw(text);
        // Rysuj przycisk i tekst na przycisku
        window.draw(button);
        window.draw(buttonText);
        window.draw(isAdminRectangle);
        window.draw(isAdmintext);
        window.draw(league_of_legends_game_status);
        league_of_legends_game_status_text.setString(league_of_legends_game_status_string);
        window.draw(league_of_legends_game_status_text);
        window.display();
    }

    // Zakoñcz w¹tek metronomu
    metronomeThread.join();
    testThread.join();
    statusUpdater.join();

    return 0;
}



//#include <SFML/Graphics.hpp>
//#include <SFML/Audio.hpp>
//#include <iostream>
//
//int main()
//{
//    sf::RenderWindow window(sf::VideoMode(800, 600), "Metronom w SFML");
//    sf::Event event;
//
//    // Inicjalizacja dŸwiêku metronomu
//    sf::SoundBuffer metronomeSoundBuffer;
//    if (!metronomeSoundBuffer.loadFromFile("C:\\Users\\kmrka\\source\\repos\\LeagueOfLegendsBPMsfml\\x64\\Debug\\sound.ogg")) {
//        std::cerr << "Failed to load metronome sound." << std::endl;
//        return 1;
//    }
//
//    sf::Sound metronomeSound;
//    metronomeSound.setBuffer(metronomeSoundBuffer);
//
//    // Ustawienia metronomu
//    float bpm = 60.0f; // Tempo w uderzeniach na minutê
//    float intervalInSeconds = 60.0f / bpm; // Interwa³ w sekundach
//
//    sf::RectangleShape metronomeRect(sf::Vector2f(50, 50));
//    metronomeRect.setFillColor(sf::Color::Red);
//
//    sf::Clock clock;
//    float elapsedTime = 0.0f;
//
//    while (window.isOpen())
//    {
//        while (window.pollEvent(event))
//        {
//            if (event.type == sf::Event::Closed)
//                window.close();
//        }
//
//        // Zliczanie czasu
//        elapsedTime += clock.restart().asSeconds();
//
//        // Odtwarzaj dŸwiêk metronomu i resetuj czas, gdy up³yn¹³ interwa³
//        if (elapsedTime >= intervalInSeconds)
//        {
//            metronomeSound.play();
//            elapsedTime = 0.0f;
//        }
//
//        window.clear();
//        window.draw(metronomeRect);
//        window.display();
//    }
//
//    return 0;
//}



//#include <iostream>
//
//#include <SFML/Audio.hpp>
//#include <SFML/Graphics.hpp>
//
//
//int main()
//{
//    sf::RenderWindow window(sf::VideoMode(800, 600), "Odtwarzanie audio");
//    sf::Event event;
//    sf::Music music;
//
//    if (!music.openFromFile("C:\\Users\\kmrka\\source\\repos\\LeagueOfLegendsBPMsfml\\x64\\Debug\\sound.ogg")) {
//        // Obs³uga b³êdu, jeœli plik nie mo¿e byæ otwarty
//        std::cout << "sad" << std::endl;
//    }
//
//    music.play();
//    sf::Clock clock;
//    sf::Time duration = sf::milliseconds(1000);
//    while (window.isOpen()) {
//        music.play();
//        while (window.pollEvent(event)) {
//            if (event.type == sf::Event::Closed) {
//                window.close();
//            }
//        }
//
//        // SprawdŸ, czy min¹³ okreœlony czas
//        if (clock.getElapsedTime() >= duration) {
//            music.stop();  // Zatrzymaj odtwarzanie po up³ywie okreœlonego czasu
//        }
//
//        window.clear();
//        window.display();
//    }
//    return 0;
//}
