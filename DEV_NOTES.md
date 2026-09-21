Turbo Spectrum / ESPectrum — sesiunea de lucru
1. ROM custom 48K
- Am verificat include/roms/rom48Kcustom.h.
- Fișierul conține un ROM custom de 16 KB reprezentat ca array C:
  gb_rom_0_48k_custom[].
- ESPConfig.cpp îl selectează pentru arhitectura 48Kcs:
  MemESP::rom[0] = (uint8_t *) gb_rom_0_48k_custom;
  și apoi ROM-ul este deplasat cu 8 bytes (+= 8).
2. Turbo Spectrum boot
- Am lucrat cu ts_boot.bin.
- Au fost pregătite și:
  - include/roms/ts_boot.h
  - ts_boot.bin
  - ts_boot_ESPectrum.bin
  - directorul ts_bin/
- Ideea de lucru: conținutul boot ROM-ului Turbo Spectrum este folosit pentru a construi/testa ROM-ul custom în ESPectrum.
- Testarea pe placa fizică rămâne pentru moment neefectuată, deoarece placa nu este disponibilă.
3. Verificarea arhitecturii în ESPectrum
Am urmărit unde este verificată Config::arch, pentru a înțelege fluxul:
- src/ESPConfig.cpp — selectarea ROM-urilor și inițializarea arhitecturii.
- src/MemESP.cpp — ramCurrent, bankLatch, pagingLock, pagingmode2A3.
- src/Ports.cpp — paging, porturi și comportamentul specific modelelor.
- src/OSDMain.cpp — meniul și selecția arhitecturii.
- src/CPU.cpp — configurarea CPU în funcție de arhitectură.
- src/Video.cpp — comportamentul video în funcție de arhitectură.
- src/ESPectrum.cpp — inițializare și timing specific arhitecturii.
- src/Snapshot.cpp — detectarea/menținerea arhitecturii la snapshot-uri.
4. Observație importantă pentru continuare
Modelul de lucru este:
1. se alege arhitectura la pornire;
2. codul verifică Config::arch în punctele unde comportamentul diferă;
3. pentru ROM custom se selectează ROM-ul corespunzător;
4. pentru Turbo Spectrum trebuie urmărit separat ce comportament hardware trebuie emulat în RAM/paging/porturi/CPU/video, fără a modifica inutil codul existent.
5. Git — salvare proiect
A fost creat remote-ul personal:
turbo -> https://github.com/nicolaebizi/turbo-spectrum-espectrum.git
Commit deja făcut:
a9f4736 Add Turbo Spectrum custom 16K ROM
Commitul conține modificarea:
include/roms/rom48Kcustom.h
A fost făcut push cu:
git push -u turbo master
Rezultat:
- branch-ul local master urmărește acum turbo/master;
- proiectul a fost publicat în repository-ul personal Turbo Spectrum.
6. Fișiere încă necomise la finalul sesiunii
Conform ultimului git status, acestea erau untracked:
- .DS_Store
- .vscode/settings.json
- drive/
- include/roms/ts_boot.h
- ts_bin/
- ts_boot.bin
- ts_boot_ESPectrum.bin
Nu le adăugăm automat până nu decidem exact ce trebuie păstrat în repository.
Următorul pas
Continuăm de aici, fără să schimbăm ce funcționează deja:
1. verificăm exact conținutul rom48Kcustom.h;
2. stabilim cum trebuie introdus ts_boot.bin în ROM-ul custom;
3. verificăm pornirea ca Spectrum 48K fără modificări suplimentare;
4. abia apoi modificăm emularea hardware Turbo Spectrum, dacă este necesar.
25 06 
Turbo Spectrum — FE Memory Mapping
Turbo Spectrum memory mapping is controlled by FE[6:5].
The reference VHDL mapping is:
case FE_CFG(6 downto 5) is

  when "00" =>
    case cpu_a(15 downto 14) is
      when "01" => ram_page <= "000";
      when "10" => ram_page <= "001";
      when "11" => ram_page <= "101";
      when others => ram_page <= "000";
    end case;

  when "01" =>
    case cpu_a(15 downto 14) is
      when "00" => ram_page <= "110";
      when "01" => ram_page <= "000";
      when "10" => ram_page <= "001";
      when "11" => ram_page <= "101";
      when others => ram_page <= "000";
    end case;

  when "11" =>
    case cpu_a(15 downto 14) is
      when "00" => ram_page <= "001";
      when "01" => ram_page <= "101";
      when "10" => ram_page <= "110";
      when "11" => ram_page <= "000";
      when others => ram_page <= "000";
    end case;

end case;


## 7. Turbo Spectrum — diagnosticarea mapării ROM/RAM și problema de write în RAM6

În această etapă am trecut de la simpla încărcare a ROM-ului Turbo Spectrum la emularea efectivă a mecanismului hardware prin care ROM-ul este dezactivat și memoria RAM îl înlocuiește.

### 7.1. Ce trebuia emulat

Conform hardware-ului Turbo Spectrum, biții `FE[6:5]` controlează maparea memoriei.

Maparea relevantă pentru boot este:

```text
FE_CFG = 00
$0000-$3FFF -> ROM
$4000-$7FFF -> RAM0
$8000-$BFFF -> RAM1
$C000-$FFFF -> RAM5

FE_CFG = 20h  (biții 6:5 = 01)
$0000-$3FFF -> RAM6
$4000-$7FFF -> RAM0
$8000-$BFFF -> RAM1
$C000-$FFFF -> RAM5

FE_CFG = 60h  (biții 6:5 = 11)
$0000-$3FFF -> RAM1
$4000-$7FFF -> RAM5
$8000-$BFFF -> RAM6
$C000-$FFFF -> RAM0
Important: în modul inițial FE_CFG=00, pagina CPU $0000-$3FFF este ROM. După trecerea în FE_CFG=20, aceeași zonă de adrese CPU $0000-$3FFF trebuie să devină RAM6.
7.2. Separarea FE_REG / FE_CFG
Am introdus două stări distincte:
static uint8_t turboFE_REG;
static uint8_t turboFE_CFG;
turboFE_REG reprezintă valoarea scrisă de CPU în portul FE, iar turboFE_CFG reprezintă configurația efectiv activată pentru maparea memoriei.
A fost adăugată funcția:
static void TurboSpectrumApplyMemoryMap();
care actualizează ramCurrent[] conform valorii turboFE_CFG.
Pentru FE_CFG=00:
ramCurrent[0] = ram[0];   // în implementarea emulatorului, aici este ROM-ul
ramCurrent[1] = ram[0];
ramCurrent[2] = ram[1];
ramCurrent[3] = ram[5];
Pentru FE_CFG=20:
ramCurrent[0] = ram[6];
ramCurrent[1] = ram[0];
ramCurrent[2] = ram[1];
ramCurrent[3] = ram[5];
Pentru FE_CFG=60:
ramCurrent[0] = ram[1];
ramCurrent[1] = ram[5];
ramCurrent[2] = ram[6];
ramCurrent[3] = ram[0];
În implementarea finală trebuie păstrat faptul că pointerul ramCurrent[0] poate indica ROM sau RAM în funcție de starea Turbo Spectrum.
7.3. De ce nu trebuia făcută remaparea la fiecare IN
La început Ports::input() copia:
turboFE_CFG = turboFE_REG;
TurboSpectrumApplyMemoryMap();
la fiecare citire de port FE.
Acest lucru avea un efect foarte important: IN (FE) este executat foarte des în anumite contexte, iar remaparea și mesajele printf() repetate introduceau overhead suficient pentru a afecta vizibil viteza animației.
Am schimbat codul astfel încât maparea să fie aplicată numai când configurația efectivă se schimbă:
uint8_t oldCfg = MemESP::turboFE_CFG;

MemESP::turboFE_CFG = MemESP::turboFE_REG;

if (MemESP::turboFE_CFG != oldCfg) {
    printf("[TURBO IN] FE_REG=%02X -> FE_CFG=%02X\n",
           MemESP::turboFE_REG,
           MemESP::turboFE_CFG);

    MemESP::TurboSpectrumApplyMemoryMap();
}
Rezultatul observat pe hardware/emulator: viteza animației a revenit la normal. Prin urmare, remaparea repetitivă și printf() din calea frecventă de I/O reprezentau un overhead real.
7.4. Ordinea reală a boot-ului Turbo Spectrum
Secvența ROM-ului este esențială.
La început:
LD A,$20
OUT ($FE),A
Dar această scriere nu înseamnă încă faptul că CPU poate scrie în RAM6.
Ulterior, la:
$E004: IN A,($FE)
se face tranziția în modul 64K DRAM.
Abia după această tranziție începe generarea logo-ului.
Specificația Turbo Spectrum descrie explicit că după inițializări sunt copiați 7B4h bytes din EEPROM/ROM în DRAM și apoi se execută jump la $E004, unde are loc tranziția în 64K DRAM mode.
7.5. Unde apare logo-ul și de ce $3FFF/$4000 este important
Logo-ul TURBO este generat inițial în:
T_buffer = $9900-$B0FF
După generare, codul Turbo Spectrum face:
LD HL,$9900
LD DE,$3000
LD BC,$1800
LDIR
Deci sunt copiați 0x1800 bytes:
$3000 ---------------------------- $47FF
          0x1000       0x800
       $3000-$3FFF   $4000-$47FF
           RAM6          RAM0
Această observație a fost foarte importantă pentru diagnostic.
Copia nu rămâne într-o singură bancă fizică: ea începe în RAM6 și continuă în RAM0.
În plus, imediat după această copie sunt generate versiunile deplasate ale logo-ului prin rutina Rotate. Turbo Spectrum folosește aceste versiuni pre-generate pentru a face rotația animației fără a recalcula tot logo-ul la fiecare cadru.
7.6. Bug-ul care producea golurile în prima animație
Inițial am modificat MemESP::writebyte() astfel încât să permită scrierea în pagina 0 atunci când:
FE_CFG != 00
Logica era corectă din punct de vedere al mapării:
FE_CFG=00 -> pagina 0 este ROM -> write interzis
FE_CFG=20 -> pagina 0 este RAM6 -> write permis
Dar aceasta nu rezolva problema reală.
Motivul: operațiile de memorie ale CPU-ului Z80 nu foloseau în mod obligatoriu funcția generică MemESP::writebyte().
În CPU.cpp, funcția efectiv folosită de Z80 este:
Z80Ops::poke8_std()
iar pointerul este inițializat astfel:
void (*Z80Ops::poke8)(uint16_t address, uint8_t value)
    = &Z80Ops::poke8_std;
Funcția veche conținea:
IRAM_ATTR void Z80Ops::poke8_std(uint16_t address, uint8_t value) {

    uint8_t page = address >> 14;

    if (page == 0) {
        VIDEO::Draw(3, false);
        return;
    }

    VIDEO::Draw(3, MemESP::ramContended[page]);
    MemESP::ramCurrent[page][address & 0x3fff] = value;
}
Aceasta înseamnă că ORICE scriere în:
$0000-$3FFF
era abandonată.
Nu conta faptul că ramCurrent[0] fusese deja schimbat din ROM în RAM6.
Așadar exista o contradicție între două componente:
MEMORY MAP:
FE_CFG=20
$0000-$3FFF -> RAM6
        OK

CPU WRITE:
page == 0
        -> return
        -> NU SCRIE
        BUG
Aceasta explică foarte bine simptomul observat: anumite părți ale imaginii erau prezente, dar prima animație avea goluri.
7.7. Corecția făcută în poke8_std()
Funcția a fost modificată pentru a ține cont de starea Turbo Spectrum:
IRAM_ATTR void Z80Ops::poke8_std(uint16_t address, uint8_t value) {

    uint8_t page = address >> 14;

    // Turbo Spectrum:
    // FE_CFG=00 -> $0000-$3FFF este ROM: writes sunt ignorate.
    // FE_CFG!=00 -> $0000-$3FFF este RAM:
    //               writes sunt permise.
    if (page == 0) {
        if (Config::romSet48 == "48Kcs" &&
            ((MemESP::turboFE_CFG & 0x60) != 0x00)) {

            VIDEO::Draw(3, false);
            MemESP::ramCurrent[page][address & 0x3fff] = value;
            return;
        }

        VIDEO::Draw(3, false);
        return;
    }

    VIDEO::Draw(3, MemESP::ramContended[page]);
    MemESP::ramCurrent[page][address & 0x3fff] = value;
}
Semantica finală este:
Turbo Spectrum + FE_CFG=00:
    page 0 = ROM
    write -> ignorat

Turbo Spectrum + FE_CFG=20:
    page 0 = RAM6
    write -> acceptat

Turbo Spectrum + FE_CFG=60:
    page 0 = RAM1
    write -> acceptat
Aceasta este corecția esențială care lipsea.
7.8. De ce simptomul a fost înșelător
Problema putea părea inițial a fi o problemă de ROM mapping, deoarece:
- citirea din $0000-$3FFF după FE_CFG=20 funcționa prin ramCurrent[0];
- ROM-ul era într-adevăr dezactivat logic;
- RAM6 era într-adevăr selectată logic;
- animația pornea și o parte din imagine era corectă;
- viteza putea fi reparată separat prin eliminarea remapării repetitive.
Dar există două operații independente:
READ:
CPU -> ramCurrent[page] -> date

WRITE:
CPU -> Z80Ops::poke8_std() -> eventual scriere în ramCurrent
Faptul că READ era corect nu garantează că WRITE este corect.
În cazul nostru:
READ  $3000 -> RAM6       OK
WRITE $3000 -> return     GREȘIT
Aceasta este explicația exactă pentru care imaginea putea avea porțiuni lipsă chiar dacă logul de mapare arăta:
FE_REG=20
FE_CFG=20
MAP[0]=RAM6
7.9. Starea la finalul acestei etape
În momentul documentării:
- ROM-ul custom 48K este selectat pentru 48Kcs;
- boot-ul Turbo Spectrum este integrat;
- turboFE_REG și turboFE_CFG sunt separate;
- maparea Turbo Spectrum este aplicată prin TurboSpectrumApplyMemoryMap();
- remaparea nu mai este făcută inutil la fiecare IN (FE);
- viteza animației a revenit la normal;
- a fost identificată cauza pentru scrierile pierdute în pagina 0;
- Z80Ops::poke8_std() a fost modificat pentru ca pagina 0 să fie write-protected numai cât timp Turbo Spectrum este în FE_CFG=00;
- după trecerea la FE_CFG=20, scrierile în RAM6 sunt permise.
7.10. Regula importantă pentru viitoarele modificări
Nu trebuie tratată pagina CPU $0000-$3FFF ca fiind permanent ROM doar pentru că proiectul folosește arhitectura 48Kcs.
Pentru Turbo Spectrum, aceeași adresă CPU își schimbă natura în timpul execuției:
BOOT:
$0000-$3FFF = ROM

după IN (FE), FE_CFG=20:
$0000-$3FFF = RAM6
Prin urmare orice cod generic care are:
if (page == 0)
    return;
trebuie verificat înainte de a fi folosit în Turbo Spectrum.
Această regulă se aplică în special la:
- poke8
- poke16
- eventual operațiile bloc de memorie
- instrucțiunile Z80 care efectuează writes indirecte
- orice optimizare CPU care presupune că pagina 0 este ROM permanent.
7.11. Testul următor
După recompilare și upload:
cd /Users/nicolaemuntean/Documents/Proiecte_ok/turbo-spectrum-espectrum
pio run -e psram
pio run -e psram -t upload
pio device monitor
Trebuie urmărite două lucruri:
1. animația trebuie să rămână la viteza normală;
2. golurile din prima animație trebuie să dispară sau să se reducă exact în zona care depinde de datele copiate în $3000-$3FFF.
Dacă prima animație devine completă, diagnosticul este confirmat: problema nu era în algoritmul de animație, ci în faptul că emulatorul păstra implicit regula Spectrum clasică „pagina 0 este ROM și nu poate primi writes”, deși Turbo Spectrum transformase deja pagina 0 în RAM6.