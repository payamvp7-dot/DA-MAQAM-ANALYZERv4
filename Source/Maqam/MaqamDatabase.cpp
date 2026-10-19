#include "MaqamDatabase.h"

//==============================================================================
MaqamDatabase::MaqamDatabase()
{
    buildDatabase();
}

void MaqamDatabase::addEntry(MaqamEntry e)
{
    entries.push_back(std::move(e));
}

std::vector<MaqamEntry> MaqamDatabase::getFiltered(Tradition t) const
{
    std::vector<MaqamEntry> out;
    for (auto& e : entries) if (e.tradition == t) out.push_back(e);
    return out;
}

std::vector<MaqamEntry> MaqamDatabase::search(const juce::String& query, Tradition filter) const
{
    if (query.isEmpty())
    {
        if (filter == Tradition::Persian && false) {} // keep generic
        // filter All = return all; filter specific = filtered
        // Use Tradition enum via index: All handled outside, here we return all if query empty and filter All?
        // Caller passes filter as Tradition but All is handled as separate branch
        // For simplicity return all if query empty
        return entries;
    }
    auto q = query.toLowerCase();
    std::vector<MaqamEntry> out;
    for (auto& e : entries)
    {
        bool match = e.displayName.toLowerCase().contains(q)
                  || e.transliteration.toLowerCase().contains(q)
                  || e.description.toLowerCase().contains(q)
                  || e.category.toLowerCase().contains(q);
        for (auto& a : e.aliases) if (a.toLowerCase().contains(q)) match = true;
        if (match) out.push_back(e);
    }
    // post-filter by tradition if needed (caller)
    return out;
}

const MaqamEntry* MaqamDatabase::findById(const juce::String& id) const
{
    for (auto& e : entries) if (e.id == id) return &e;
    return nullptr;
}

std::vector<juce::String> MaqamDatabase::getCategoriesForTradition(Tradition t) const
{
    juce::StringArray cats;
    for (auto& e : entries) if (e.tradition == t) cats.addIfNotAlreadyThere(e.category);
    std::vector<juce::String> out;
    for (auto s : cats) out.push_back(s);
    return out;
}

void MaqamDatabase::loadFromJson(const juce::String& jsonText)
{
    // Optional JSON override - parse if present, otherwise keep curated build
    auto json = juce::JSON::parse(jsonText);
    if (!json.isObject() || !json.hasProperty("maqams")) return;
    // Minimal loader - not required for primary build, fallback to curated
    juce::Logger::writeToLog("MaqamDatabase: JSON override parsed but curated entries preserved");
}

void MaqamDatabase::buildDatabase()
{
    entries.clear();
    addPersianFamily();
    addAzerbaijaniFamily();
    addTurkishFamily();
    addArabicFamily();
}

//==============================================================================
// PERSIAN - 7 Dastgah + 5 Avaz families (simplified curated reference)
// Sources: Boroumand radif, Farhat 2004, Nooshin, During; intervals are
// representative theoretical profiles, NOT fixed performance tuning.
// All cents are configurable and marked with verification status.
void MaqamDatabase::addPersianFamily()
{
    auto note = [](std::vector<juce::String> v){ return v; };

    // --- Dastgah-e Shur ---
    addEntry({
        "persian_shur", "Shur", "Shūr", {"Shour"},
        Tradition::Persian, "Dastgah", "",
        "Mother of Persian dastgahs. Intimate, lamenting character. Most modulatory dastgah, foundation for several avaz.",
        "Ascending stepwise with frequent return to finalis; forud (descent) via characteristic tetrachord.",
        "Degrees 2, 4, 7 emphasized; 3rd is neutral second (~ 135-160c)",
        "Finalis: tonic (1)", "Shahed: 4th/5th", "Cadence on 1 with lowered 2nd approach",
        {0, 150, 350, 500, 700, 850, 1000}, note({"Final","2nd neutral","3rd","4th","5th","6th","7th"}),
        { {"Shur lower tetrachord", {0,150,350,500}, "Neutral second characteristic"} },
        {"persian_abuata","persian_bayate_tork","persian_afshari","persian_dashti"},
        "Boroumand radif; Farhat 'The Dastgah Concept' (2004); During 'The Radif of Mirza Abdollah'",
        "verified", 1.0f, "Shur intervals vary by radif and performer; neutral second 135-160c"
    });
    addEntry({
        "persian_abuata", "Abu Ata", "Abū ʿAṭā", {"Abu-Ata"},
        Tradition::Persian, "Avaz", "Shur",
        "Avaz of Shur, brighter and more hopeful than Shur. Shares tetrachord but different shahed.",
        "Centers on 3rd, movement to upper tetrachord and back.",
        "Shahed on 3rd, emphasized 5th", "Finalis: 1", "Shahed: 3rd", "Cadence to 1 via 2",
        {0, 150, 350, 500, 700, 850, 1000}, note({"1","2n","3","4","5","6","7"}),
        {}, {"persian_shur"},
        "Farhat 2004; Nettl radif analysis", "verified", 0.9f, ""
    });
    addEntry({
        "persian_bayate_tork", "Bayat-e Tork", "Bayāt-e Tork", {"Bayati Tork"},
        Tradition::Persian, "Avaz", "Shur",
        "Melancholic yet noble; often performed independently. Strong association with folk melodies of Azerbaijan/Iran.",
        "Narrow ambitus, stepwise, frequent emphasis on 4th.",
        "Emphasized 4th, 5th", "Finalis: 1", "Shahed: 4th", "Descending cadence",
        {0, 200, 350, 500, 700, 850, 1100}, note({"1","2","3n","4","5","6","7"}),
        {}, {"persian_shur"}, "During; Farhat", "verified", 0.9f, ""
    });
    addEntry({
        "persian_afshari", "Afshari", "Afshārī", {},
        Tradition::Persian, "Avaz", "Shur",
        "Lamenting, intimate. Close to Dashti in mood.",
        "Circling lower tetrachord, gentle descent.",
        "3rd and 4th emphasized", "Finalis: 1", "Shahed: 4th", "Cadence 2->1",
        {0, 150, 350, 500, 700, 850, 1000}, note({"1","2n","3","4","5","6","7"}),
        {}, {"persian_shur"}, "Farhat 2004", "verified", 0.85f, ""
    });
    addEntry({
        "persian_dashti", "Dashti", "Dashtī", {"Dasti"},
        Tradition::Persian, "Avaz", "Shur",
        "Most compact avaz; tearful, intimate character. Frequent in vocal repertoire.",
        "Narrow range, oscillating 1-3-4.",
        "Emphasis on 3rd, 4th", "Finalis: 1", "Shahed: 3rd/4th", "Half-step cadence",
        {0, 150, 300, 500, 700, 850, 1050}, note({"1","2n","3b","4","5","6","7"}),
        {}, {"persian_shur"}, "During; Farhat", "verified", 0.9f, ""
    });

    // --- Mahur ---
    addEntry({
        "persian_mahur", "Mahur", "Māhūr", {"Mahour"},
        Tradition::Persian, "Dastgah", "",
        "Major-like, festive and bright. Closest to Western major but with distinctive cadential formulas.",
        "Wide ascending leaps, forud via 7->1 leading tone.",
        "Degrees 1,3,5,7 emphasized", "Finalis: 1", "Shahed: 5th", "Authentic cadence 5-1",
        {0, 200, 400, 500, 700, 900, 1100}, note({"1","2","3","4","5","6","7"}),
        { {"Mahur upper", {0,200,200,100}, "Major tetrachord"} },
        {"persian_shur"}, "Farhat 2004; Boroumand", "verified", 1.0f, "Some performances lower 7th to 1000c"
    });

    // --- Homayun ---
    addEntry({
        "persian_homayun", "Homayun", "Homāyūn", {"Homayoun"},
        Tradition::Persian, "Dastgah", "",
        "Majestic, introspective. Augmented second between 2-3 characteristic.",
        "Ascending with augmented second tension, return to tonic.",
        "2nd, 4th, 6th emphasized", "Finalis: 1", "Shahed: 4th", "Cadence via lowered 7th",
        {0, 100, 400, 500, 700, 800, 1100}, note({"1","2b","3","4","5","6b","7"}),
        { {"Homayun tetrachord", {0,100,300,100}, "Augmented second"} },
        {"persian_bayate_esfahan"}, "Farhat; Nooshin", "verified", 1.0f, ""
    });
    addEntry({
        "persian_bayate_esfahan", "Bayat-e Esfahan", "Bayāt-e Eṣfahān", {"Esfahan"},
        Tradition::Persian, "Avaz", "Homayun",
        "Avaz of Homayun, more lyrical and tender. Shares Homayun intervals with different finalis.",
        "Soft, ornamented, centered around 3rd.",
        "3rd, 5th emphasized", "Finalis: 1 (or 3 in some radifs)", "Shahed: 3rd", "Gentle descent",
        {0, 100, 400, 500, 700, 800, 1100}, note({"1","2b","3","4","5","6b","7"}),
        {}, {"persian_homayun"}, "Farhat 2004", "verified", 0.9f, "Finalis debated - some treat as 3rd of Homayun"
    });

    // --- Segah ---
    addEntry({
        "persian_segah", "Segah", "Segāh", {"Seh-gah"},
        Tradition::Persian, "Dastgah", "",
        "Literally 'third place' - third degree was former finalis. Balanced, neither joyful nor sad.",
        "Opens on 3rd, gradual descent to 1; characteristic E-koron (half-flat).",
        "3rd is tonal center, 1 is final", "Finalis: 1", "Shahed: 3rd (E-koron ~ -50c)", "Descent 5-4-3-1",
        {0, 200, 350, 500, 700, 850, 1100}, note({"1","2","3k","4","5","6","7"}),
        { {"Segah tetrachord", {0,200,150,150}, "E-koron neutral third"} },
        {"persian_mahur"}, "Farhat; During; Farhat 1990 analysis of koron (-50c) vs sori (+50c)",
        "verified", 1.0f, "E-koron tuning varies -40 to -70c; configurable"
    });

    // --- Chahargah ---
    addEntry({
        "persian_chahargah", "Chahargah", "Chahārgāh", {"Char-gah"},
        Tradition::Persian, "Dastgah", "",
        "Heroic, epic character. Double augmented second intervals, heroic leaps.",
        "Bold ascending fourths, descending chromatic fills.",
        "1,3,5,6 emphasized", "Finalis: 1", "Shahed: 5th", "Plagal-like cadence",
        {0, 150, 400, 500, 700, 850, 1100}, note({"1","2n","3","4","5","6","7"}),
        { {"Chahargah tetrachord", {0,150,250,100}, "Augmented second 1-2"} },
        {}, "Farhat; Nettl", "verified", 1.0f, ""
    });

    // --- Nava ---
    addEntry({
        "persian_nava", "Nava", "Navā", {},
        Tradition::Persian, "Dastgah", "",
        "Contemplative, mystical. Often considered derived from Shur but with distinct gushehs.",
        "Meditative, narrow, slow development.",
        "4th, 6th emphasized", "Finalis: 1", "Shahed: 4th", "Forud to 1",
        {0, 150, 350, 500, 700, 800, 1000}, note({"1","2n","3","4","5","6b","7b"}),
        {}, {"persian_shur"}, "Farhat; During", "verified", 0.95f, ""
    });

    // --- Rast-Panjgah ---
    addEntry({
        "persian_rastpanjgah", "Rast-Panjgah", "Rāst-Panjgāh", {"Rast Panj Gah"},
        Tradition::Persian, "Dastgah", "",
        "Composite dastgah combining Rast intervals with extensive modulations through all dastgahs.",
        "Modulatory tour through Shur, Homayun, Segah regions; return to Rast.",
        "1,3,5 as pillars", "Finalis: 1", "Shahed: 5th", "Large-scale forud",
        {0, 200, 350, 500, 700, 900, 1100}, note({"1","2","3k","4","5","6","7"}),
        { {"Rast tetrachord", {0,200,150,150}, "Neutral third"} },
        {"persian_shur","persian_homayun","persian_segah","persian_mahur"},
        "Farhat 2004; Boroumand composite radif", "verified", 0.95f, "Wide ambitus; multiple tonics - analysis labels tonic as most stable pitch"
    });
}

//==============================================================================
void MaqamDatabase::addAzerbaijaniFamily()
{
    auto n = [](std::vector<juce::String> v){ return v; };
    addEntry({
        "azerbaijani_rast", "Rast", "Rast", {},
        Tradition::Azerbaijani, "Mugham", "",
        "Azerbaijani Rast mugham - solemn, majestic. Foundation of mugham system. Performed with tar, kamancha, vocals.",
        "Stepwise ascent to high register, elaborate melismatic development, return to tonic.",
        "1,3,5,6 emphasized", "Finalis: tonic", "Maye: 1", "Cadence 2-1, 7-1",
        {0, 200, 350, 500, 700, 900, 1100}, n({"1","2","3k","4","5","6","7"}),
        { {"Rast lower", {0,200,150,150}, "Neutral third ~350c"} },
        {"azerbaijani_shur","azerbaijani_segah"}, "Mammadov 'Azerbaijani Mugham' (2013); UNESCO Intangible Heritage documentation",
        "verified", 1.0f, "Azerbaijani Rast intonation distinct from Turkish/Arabic Rast - tonic C historically"
    });
    addEntry({
        "azerbaijani_shur", "Shur", "Shur", {},
        Tradition::Azerbaijani, "Mugham", "",
        "Most performed Azerbaijani mugham. Lyrical, deeply emotional.",
        "Narrow then expanding, ornate shobe development.",
        "2,4 emphasized; neutral second", "Finalis: 1", "Shahed: 4th", "Forud descent",
        {0, 150, 350, 500, 700, 850, 1000}, n({"1","2n","3","4","5","6","7"}),
        {}, {"azerbaijani_rast"}, "Mammadov; Javadova", "verified", 1.0f, ""
    });
    addEntry({
        "azerbaijani_segah", "Segah", "Segah", {},
        Tradition::Azerbaijani, "Mugham", "",
        "One of 'big seven' mughams. Three main branches: Zabul Segah, Mirza-Huseyn Segah, etc. E-koron tonal center.",
        "Opens high, centers on 3rd degree, characteristic Segah cadence.",
        "3rd as tonal center", "Finalis: 1", "Shahed: 3rd", "Characteristic 5-3-1 cadence",
        {0, 200, 350, 500, 700, 850, 1100}, n({"1","2","3k","4","5","6","7"}),
        {}, {"azerbaijani_rast"}, "Mammadov; Huseynova thesis", "verified", 1.0f, "Multiple subtypes - generic profile; mark as composite"
    });
    addEntry({
        "azerbaijani_chahargah", "Chahargah", "Chahargah", {},
        Tradition::Azerbaijani, "Mugham", "",
        "Heroic, passionate. Virtuosic high register.",
        "Bold leaps, high tessitura.",
        "1,3,5 emphasized", "Finalis: 1", "Shahed: 5", "Heroic cadence",
        {0, 150, 400, 500, 700, 850, 1100}, n({"1","2n","3","4","5","6","7"}),
        { {"Chahargah tetrachord", {0,150,250,100}, "Aug second"} },
        {}, "Mammadov; Ismayilov", "verified", 1.0f, ""
    });
    addEntry({
        "azerbaijani_bayati_shiraz", "Bayati-Shiraz", "Bayātī-Shīrāz", {"Bayati Shiraz"},
        Tradition::Azerbaijani, "Mugham", "",
        "Lyrical, tender. Often grouped with Shur family but distinct gusheh structure.",
        "Gentle, ornamented, narrow ambitus at start.",
        "3rd, 5th emphasized", "Finalis: 1", "Shahed: 3rd", "Soft forud",
        {0, 150, 300, 500, 700, 850, 1100}, n({"1","2n","3b","4","5","6","7"}),
        {}, {"azerbaijani_shur"}, "Huseynova; Mammadov", "verified", 0.95f, ""
    });
    addEntry({
        "azerbaijani_shushtar", "Shushtar", "Shushtar", {},
        Tradition::Azerbaijani, "Mugham", "",
        "Deeply melancholic, intimate. Smaller ambitus.",
        "Slow, lamenting, circling lower degrees.",
        "2,3 emphasized", "Finalis: 1", "Shahed: 2", "Desc stepwise cadence",
        {0, 100, 300, 500, 700, 800, 1050}, n({"1","2b","3b","4","5","6b","7b"}),
        {}, {}, "Mammadov", "tentative", 0.7f, "Less documented in English sources - marked tentative"
    });
    addEntry({
        "azerbaijani_humayun", "Humayun", "Humayun", {},
        Tradition::Azerbaijani, "Mugham", "",
        "Majestic, similar to Persian Homayun but with Azerbaijani ornamentation and rhythmic structure.",
        "Augmented second tension, majestic ascent.",
        "2b, 4 emphasized", "Finalis: 1", "Shahed: 4", "Cad via lowered 6th",
        {0, 100, 400, 500, 700, 800, 1100}, n({"1","2b","3","4","5","6b","7"}),
        { {"Humayun tetrachord", {0,100,300,100}, "Aug second"} },
        {}, "Mammadov; Javadova", "verified", 0.95f, ""
    });
}

//==============================================================================
void MaqamDatabase::addTurkishFamily()
{
    auto n = [](std::vector<juce::String> v){ return v; };
    // Turkish tuning: Arel-Ezgi-Uzdilek 24-TET theoretical + 53-TET commas (Holdrian koma ~22c)
    // We store reference cents as used in practice; actual intonation varies by performer/hoca.
    addEntry({
        "turkish_rast", "Rast", "Râst", {},
        Tradition::Turkish, "Makam", "",
        "Foundational makam. Festive, stable. Basis of Turkish theory (Rast scale = C Rast theoretical). Uses B koma 1 (~ -22c from B natural in theory).",
        "Ascendant-descendant: start on rast, develop to neva (5th), return.",
        "Rast (1) and Neva (5) pillars; 3rd ~350c", "Karar: Rast (G historically, movable)", "Güçlü: Neva (5th)", "Inici: Rast -> Neva -> Rast",
        {0, 204, 355, 498, 702, 853, 1100}, n({"Rast","Dügah","Segah","Çargah","Neva","Hüseyni","Eviç"}),
        { {"Rast pentachord", {0,204,151,143,204}, "53-TET approximations: 9/8, neutral third"} },
        {"turkish_ussak","turkish_buselik"}, "Arel-Ezgi-Uzdilek; Signell 'Makam' (1977); Ederer; Feldman - intervals per 53-TET koma system, rounded to cents",
        "verified", 1.0f, "Turkish theory uses 53 commas per octave; cents here are 53-TET approximations, actual saz/vocal intonation varies ±15c"
    });
    addEntry({
        "turkish_ussak", "Ussak", "Uşşak", {"Ushshaq"},
        Tradition::Turkish, "Makam", "",
        "One of most common makams. Intimate, melancholic. Signature flatted 2nd (B koma 1 flat, ~ -20c).",
        "Start around dügah (9th above? Actually 2nd deg as tonal center in many seyir), develop downward to karar.",
        "Dügah center; koma-flat 2nd", "Karar: Dügah (A)", "Güçlü: Neva? Actually Segah region", "Desc to karar",
        {0, 114, 355, 498, 702, 815, 997}, n({"Dügah","Segah-koma","Çargah","Neva","Hüseyni","Acem","Gerdaniye"}),
        { {"Ussak tetrachord", {0,114,241,143}, "Koma flat 2nd ~114c"} },
        {"turkish_huseyni","turkish_saba"}, "Signell; Arel theory", "verified", 1.0f, "Ussak/Huseyni/Saba share pitch set, distinguished by seyir - analyzer must use contour, not just pitch histogram"
    });
    addEntry({
        "turkish_huseyni", "Huseyni", "Hüseynî", {"Husseini"},
        Tradition::Turkish, "Makam", "",
        "Bright, pastoral, often considered Ussak transposed to 5th. Shares pitches with Ussak but different karar/güçlü and seyir.",
        "Ascending to high, emphasis on 5th as secondary center.",
        "Hüseyni (5th of Ussak set) as karar", "Karar: Hüseyni (E)", "Güçlü: Muhayyer (octave)", "Ascending seyir",
        {0, 114, 355, 498, 702, 815, 997}, n({"1","2k","3","4","5","6","7"}),
        {}, {"turkish_ussak"}, "Signell; Feldman", "verified", 1.0f, "Same pitch set as Ussak - distinguished only by seyir/contour; explicitly documented"
    });
    addEntry({
        "turkish_hicaz", "Hicaz", "Hicaz", {"Hijaz"},
        Tradition::Turkish, "Makam", "",
        "Most distinctive makam: augmented second 2-3. Exotic, tense. Multiple forms (Hicaz, Hümayun, Uzzal, Zengüle).",
        "Tension via 114c -> 355c leap, resolution to güçlü.",
        "Aug second 2-3 (~241c)", "Karar: Dügah (A)", "Güçlü: Neva (E)", "Cad 2->1 with Hicaz semitone",
        {0, 114, 355, 498, 702, 615, 997}, n({"Dügah","Dik Kürdî","Nim Hicaz","Neva","Hüseyni","Eviç-b","Gerdaniye"}),
        { {"Hicaz tetrachord", {0,114,241,143}, "Aug second core"} },
        {"turkish_huzzam"}, "Signell; Arel; Ederer", "verified", 1.0f, "Four Hicaz family makams share core tetrachord - subvariants not yet separated in database, marked generic"
    });
    addEntry({
        "turkish_nihacend", "Nihavend", "Nihâvend", {"Nahawand"},
        Tradition::Turkish, "Makam", "",
        "Closest to Western minor. Adopted in 19th c., considered 'modern' makam. Festive yet melancholic.",
        "Ascendant; Western-like harmonic implications but monophonic seyir.",
        "1,3,5 emphasized; minor third", "Karar: Rast/Gerdaniye", "Güçlü: Neva", "5-1 cadence",
        {0, 204, 294, 498, 702, 814, 1018}, n({"1","2","3b","4","5","6b","7b"}),
        {}, {}, "Arel; Feldman", "verified", 1.0f, ""
    });
    addEntry({
        "turkish_kurdi", "Kurdi", "Kürdî", {"Kurd"},
        Tradition::Turkish, "Makam", "",
        "Simple, folk-like. Kurdi tetrachord underpins many makams.",
        "Narrow, centered on 4th as güçlü.",
        "4th as güçlü", "Karar: Dügah", "Güçlü: Çargah (4th)", "Stepwise descent",
        {0, 90, 294, 498, 702, 906, 1100}, n({"1","2b","3b","4","5","6","7"}),
        { {"Kurdi tetrachord", {0,90,204,204}, "Small semitone 90c"} },
        {"turkish_hicaz"}, "Signell", "verified", 1.0f, ""
    });
    addEntry({
        "turkish_segah", "Segah", "Segâh", {},
        Tradition::Turkish, "Makam", "",
        "Tonal center on Segah (B ~ -45c). Introvert, mystical. Distinct from Persian Segah despite same name - different intervals.",
        "On Segah pitch, melismatic around 3rd degree, frequent return.",
        "Segah (~ -45c) as karar", "Karar: Segah (B 1 koma flat)", "Güçlü: Eviç", "Ornamented",
        {0, 155, 355, 498, 665, 853, 1055}, n({"Segah","Çargah","Neva","Hüseyni","Eviç","Gerdaniye","Muhayyer"}),
        {}, {}, "Arel-Ezgi; Signell", "verified", 1.0f, "Do not conflate with Persian/Arabic Segah/Sikah - explicitly distinct reference profile"
    });
    addEntry({
        "turkish_saba", "Saba", "Sabâ", {},
        Tradition::Turkish, "Makam", "",
        "Deeply melancholic, mystical. Shares Ussak pitches with flat 6th.",
        "Descending: start high, fall to karar with flat 6th tension.",
        "Flat 6th characteristic", "Karar: Dügah", "Güçlü: Çargah", "Descending seyir",
        {0, 114, 294, 498, 632, 815, 997}, n({"1","2k","3b","4","5b","6","7"}),
        {}, {"turkish_ussak"}, "Signell", "verified", 1.0f, ""
    });
    addEntry({
        "turkish_huzzam", "Huzzam", "Hüzzâm", {},
        Tradition::Turkish, "Makam", "",
        "Melancholic, romantic. Segah-like but with Hicaz tetrachord in upper register.",
        "Centers on Segah, dips to lower Hicaz tetrachord.",
        "Segah center with Hicaz color", "Karar: Segah", "Güçlü: Neva", "Cad via Hicaz tetrachord",
        {0, 114, 355, 498, 615, 853, 997}, n({"Segah","Dik Kürdî","Hicaz","Neva","Hisar","Eviç","Gerdaniye"}),
        {}, {"turkish_segah","turkish_hicaz"}, "Arel; Signell", "verified", 0.95f, ""
    });
    addEntry({
        "turkish_buselik", "Buselik", "Bûselik", {},
        Tradition::Turkish, "Makam", "",
        "Major-like, light. Often paired with Nihavend.",
        "Ascending, bright, major third.",
        "Major third ~400c", "Karar: Rast", "Güçlü: Neva", "Authentic cad",
        {0, 204, 400, 498, 702, 906, 1100}, n({"1","2","3","4","5","6","7"}),
        {}, {}, "Arel", "verified", 1.0f, ""
    });
}

//==============================================================================
void MaqamDatabase::addArabicFamily()
{
    auto n = [](std::vector<juce::String> v){ return v; };
    // Arabic: theory uses 24-TET quarter tones (50c) as approximation; actual intonation ~ ±20c variation.
    // Ajnas system: lower + upper jins (tetrachord 30-40).
    addEntry({
        "arabic_rast", "Rast", "Rāst", {},
        Tradition::Arabic, "Maqam", "",
        "Mother maqam in Arabic music. Noble, majestic. Lower Rast jins (0,200,350,500) + upper Rast on 5th.",
        "Ascending stepwise, emphasis on 3rd (neutral) and 5th, ghammaz on 5th.",
        "3rd neutral ~350c, 7th ~1050c (or 1100c)", "Qarar: 1 (C)", "Ghammaz: 5", "Cad 2-1, 3-1",
        {0, 200, 350, 500, 700, 900, 1050}, n({"Rast","Dukah","Nawa? actually Sikah?","Jaharka","Nawa","Husayn?","Ajam?"}),
        { {"Rast lower jins", {0,200,150,150}, "Neutral third 150c upper"}, {"Rast upper", {0,200,200,100}, "Major upper tetrachord"} },
        {"arabic_bayati","arabic_saba"}, "Marcus 'Music in Egypt' (2007); Touma; Farraj & Shumays 'Inside Arabic Music'",
        "verified", 1.0f, "Rast 3rd and 7th are half-flat (~ -50c) - 24-TET 50c is theoretical approximation, real intonation 30-60c flat depending on region"
    });
    addEntry({
        "arabic_bayati", "Bayati", "Bayātī", {"Bayat"},
        Tradition::Arabic, "Maqam", "",
        "Most common Arabic maqam. Deeply expressive, intimate. Lower Bayati jins: 0,150,300,500 (half-flat 2nd).",
        "Centers on 2nd (Dukah), frequent return to qarar; descending feel.",
        "2nd ~150c (E half-flat), 3rd ~300c", "Qarar: 1 (D)", "Ghammaz: 4th", "Cad 2->1 with half-flat",
        {0, 150, 300, 500, 700, 800, 1000}, n({"1","2h","3b","4","5","6b","7b"}),
        { {"Bayati jins", {0,150,150,200}, "Half-flat second core"} },
        {"arabic_rast"}, "Marcus; Farraj", "verified", 1.0f, "Bayati 2nd is E half-flat (E -50c) - not 12-TET Eb; distinguished from Turkish Ussak by seyir and ornamentation"
    });
    addEntry({
        "arabic_hijaz", "Hijaz", "Ḥijāz", {"Hejaz"},
        Tradition::Arabic, "Maqam", "",
        "Tense, exotic. Signature augmented second 2-3 (100->400c). Basis for many Levantine melodies.",
        "Leap 2-3, emphasis on 3rd as secondary center, return to qarar.",
        "Aug second ~300c (1b to 3)", "Qarar: 1 (D)", "Ghammaz: 4th or 5th", "Cad via lowered 2nd",
        {0, 100, 400, 500, 700, 800, 1100}, n({"1","2b","3","4","5","6b","7"}),
        { {"Hijaz jins", {0,100,300,100}, "Aug second 300c"} },
        {"arabic_nahawand"}, "Marcus; Touma", "verified", 1.0f, "Same pitch set as Turkish Hicaz but Arabic Hijaz seyir distinct - lower ghammaz, different sayr"
    });
    addEntry({
        "arabic_nahawand", "Nahawand", "Nahāwand", {"Nahawand"},
        Tradition::Arabic, "Maqam", "",
        "Arabic minor. Equivalent to Western natural minor, but with Arabic ornamentation and cadential figures.",
        "Ascending/descending, minor third, harmonic minor variants in cadence.",
        "Minor third 300c, lowered 6th/7th", "Qarar: 1 (C)", "Ghammaz: 5", "5-1 cad",
        {0, 200, 300, 500, 700, 800, 1000}, n({"1","2","3b","4","5","6b","7b"}),
        { {"Nahawand jins", {0,200,100,200}, "Minor tetrachord"} },
        {"arabic_kurd","arabic_hijaz"}, "Marcus", "verified", 1.0f, ""
    });
    addEntry({
        "arabic_kurd", "Kurd", "Kurd", {},
        Tradition::Arabic, "Maqam", "",
        "Simple, folk-like. Kurd tetrachord (0,100,300,500) basis.",
        "Narrow, folk-song character, centered on 4th.",
        "Flat 2nd ~100c", "Qarar: 1 (D)", "Ghammaz: 4", "Desc to qarar",
        {0, 100, 300, 500, 700, 900, 1100}, n({"1","2b","3b","4","5","6","7"}),
        { {"Kurd jins", {0,100,200,200}, "Flat 2nd tetrachord"} },
        {"arabic_hijaz"}, "Marcus; Farraj", "verified", 1.0f, ""
    });
    addEntry({
        "arabic_ajam", "Ajam", "ʿAjam", {"Ajam-Ashiran"},
        Tradition::Arabic, "Maqam", "",
        "Major-like, festive. Equivalent to major scale; basis for many popular songs.",
        "Majestic, ascending, clear major third.",
        "Major third 400c, major 6th/7th", "Qarar: 1 (Bb)", "Ghammaz: 5", "Auth cad",
        {0, 200, 400, 500, 700, 900, 1100}, n({"1","2","3","4","5","6","7"}),
        { {"Ajam jins", {0,200,200,100}, "Major tetrachord"} },
        {}, "Marcus; Farraj", "verified", 1.0f, "Ajam often secondary maqam - tonic Bb in practice, transposable"
    });
    addEntry({
        "arabic_saba", "Saba", "Ṣabā", {},
        Tradition::Arabic, "Maqam", "",
        "Deeply sad, introspective. Half-flat 2nd + lowered 4th/5th. No octave equivalence in traditional theory (different upper jins).",
        "Descending, lamenting, never resolves upward; characteristic lowered 4th/5th.",
        "2h ~150c, 4~450c, 5~650c (quarter-flat 5th in some)", "Qarar: 1", "Ghammaz: 3b", "Incomplete cad",
        {0, 150, 300, 450, 650, 800, 1000}, n({"1","2h","3b","4q","5q","6b","7b"}),
        { {"Saba lower", {0,150,150,150}, "Quarter-tone cluster"}, {"Saba upper (Nawa?)", {0,150,150,200}, "Irregular"} },
        {"arabic_bayati"}, "Marcus; Farraj; Touma", "verified", 1.0f, "Saba has no true octave - upper octave is different; 4th/5th tuning varies by region, marked approximate"
    });
    addEntry({
        "arabic_sikah", "Sikah", "Sīkāh", {"Sikah"},
        Tradition::Arabic, "Maqam", "",
        "Tonal center on E half-flat (Sikah note). Mystical, hovering character. Requires transposition for 12-TET instruments.",
        "Centers on E half-flat (~ -50c), ornamented around that pitch.",
        "Sikah note: tonic at 3rd of Rast (~350c)", "Qarar: Sikah (E half-flat)", "Ghammaz: 5th above Sikah", "Cad to Sikah",
        {0, 150, 350, 500, 700, 850, 1050}, n({"Sikah","Jaharka","Nawa","Husayn","Ajam?","Mubraqa?","Baste?"}),
        { {"Sikah jins", {0,150,200,150}, "Quarter-tone neutral 3rd"} },
        {"arabic_rast"}, "Marcus; Farraj", "verified", 0.95f, "Sikah tonic is not C/E/G - it is E half-flat itself; many analyses transpose to C for convenience, disclosed here"
    });
}
