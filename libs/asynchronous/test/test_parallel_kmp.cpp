// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <vector>
#include <random>
#include <future>

#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>

#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/algorithm/parallel_kmp.hpp>

#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;

namespace
{

// We need some long non-copyrighted text to run KMP on in this example, so here are the first ten chapters of Caesar's Commentarii de Bello Gallico.
std::string caesar = "Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur. Hi omnes lingua, institutis, "
                     "legibus inter se differunt. Gallos ab Aquitanis Garumna flumen, a Belgis Matrona et Sequana dividit. Horum omnium fortissimi sunt Belgae, propterea quod a cultu atque humanitate "
                     "provinciae longissime absunt, minimeque ad eos mercatores saepe commeant atque ea quae ad effeminandos animos pertinent important, proximique sunt Germanis, qui trans Rhenum "
                     "incolunt, quibuscum continenter bellum gerunt. Qua de causa Helvetii quoque reliquos Gallos virtute praecedunt, quod fere cotidianis proeliis cum Germanis contendunt, cum aut "
                     "suis finibus eos prohibent aut ipsi in eorum finibus bellum gerunt. Eorum una, pars, quam Gallos obtinere dictum est, initium capit a flumine Rhodano, continetur Garumna flumine, "
                     "Oceano, finibus Belgarum, attingit etiam ab Sequanis et Helvetiis flumen Rhenum, vergit ad septentriones. Belgae ab extremis Galliae finibus oriuntur, pertinent ad inferiorem "
                     "partem fluminis Rheni, spectant in septentrionem et orientem solem. Aquitania a Garumna flumine ad Pyrenaeos montes et eam partem Oceani quae est ad Hispaniam pertinet; spectat "
                     "inter occasum solis et septentriones.\n"
                     "Apud Helvetios longe nobilissimus fuit et ditissimus Orgetorix. Is M. Messala, [et P.] M. Pisone consulibus regni cupiditate inductus coniurationem nobilitatis fecit et civitati "
                     "persuasit ut de finibus suis cum omnibus copiis exirent: perfacile esse, cum virtute omnibus praestarent, totius Galliae imperio potiri. Id hoc facilius iis persuasit, quod undique "
                     "loci natura Helvetii continentur: una ex parte flumine Rheno latissimo atque altissimo, qui agrum Helvetium a Germanis dividit; altera ex parte monte Iura altissimo, qui est inter "
                     "Sequanos et Helvetios; tertia lacu Lemanno et flumine Rhodano, qui provinciam nostram ab Helvetiis dividit. His rebus fiebat ut et minus late vagarentur et minus facile finitimis "
                     "bellum inferre possent; qua ex parte homines bellandi cupidi magno dolore adficiebantur. Pro multitudine autem hominum et pro gloria belli atque fortitudinis angustos se fines "
                     "habere arbitrabantur, qui in longitudinem milia passuum CCXL, in latitudinem CLXXX patebant.\n"
                     "His rebus adducti et auctoritate Orgetorigis permoti constituerunt ea quae ad proficiscendum pertinerent comparare, iumentorum et carrorum quam maximum numerum coemere, sementes "
                     "quam maximas facere, ut in itinere copia frumenti suppeteret, cum proximis civitatibus pacem et amicitiam confirmare. Ad eas res conficiendas biennium sibi satis esse duxerunt; in "
                     "tertium annum profectionem lege confirmant. Ad eas res conficiendas Orgetorix deligitur. Is sibi legationem ad civitates suscipit. In eo itinere persuadet Castico, Catamantaloedis "
                     "filio, Sequano, cuius pater regnum in Sequanis multos annos obtinuerat et a senatu populi Romani amicus appellatus erat, ut regnum in civitate sua occuparet, quod pater ante "
                     "habuerit; itemque Dumnorigi Haeduo, fratri Diviciaci, qui eo tempore principatum in civitate obtinebat ac maxime plebi acceptus erat, ut idem conaretur persuadet eique filiam "
                     "suam in matrimonium dat. Perfacile factu esse illis probat conata perficere, propterea quod ipse suae civitatis imperium obtenturus esset: non esse dubium quin totius Galliae "
                     "plurimum Helvetii possent; se suis copiis suoque exercitu illis regna conciliaturum confirmat. Hac oratione adducti inter se fidem et ius iurandum dant et regno occupato per tres "
                     "potentissimos ac firmissimos populos totius Galliae sese potiri posse sperant.\n"
                     "Ea res est Helvetiis per indicium enuntiata. Moribus suis Orgetoricem ex vinculis causam dicere coegerunt; damnatum poenam sequi oportebat, ut igni cremaretur. Die constituta "
                     "causae dictionis Orgetorix ad iudicium omnem suam familiam, ad hominum milia decem, undique coegit, et omnes clientes obaeratosque suos, quorum magnum numerum habebat, eodem "
                     "conduxit; per eos ne causam diceret se eripuit. Cum civitas ob eam rem incitata armis ius suum exequi conaretur multitudinemque hominum ex agris magistratus cogerent, Orgetorix "
                     "mortuus est; neque abest suspicio, ut Helvetii arbitrantur, quin ipse sibi mortem consciverit.\n"
                     "Post eius mortem nihilo minus Helvetii id quod constituerant facere conantur, ut e finibus suis exeant. Ubi iam se ad eam rem paratos esse arbitrati sunt, oppida sua omnia, numero "
                     "ad duodecim, vicos ad quadringentos, reliqua privata aedificia incendunt; frumentum omne, praeter quod secum portaturi erant, comburunt, ut domum reditionis spe sublata paratiores "
                     "ad omnia pericula subeunda essent; trium mensum molita cibaria sibi quemque domo efferre iubent. Persuadent Rauracis et Tulingis et Latobrigis finitimis, uti eodem usi consilio "
                     "oppidis suis vicisque exustis una cum iis proficiscantur, Boiosque, qui trans Rhenum incoluerant et in agrum Noricum transierant Noreiamque oppugnabant, receptos ad se socios sibi "
                     "adsciscunt.\n"
                     "Erant omnino itinera duo, quibus itineribus domo exire possent: unum per Sequanos, angustum et difficile, inter montem Iuram et flumen Rhodanum, vix qua singuli carri ducerentur, "
                     "mons autem altissimus impendebat, ut facile perpauci prohibere possent; alterum per provinciam nostram, multo facilius atque expeditius, propterea quod inter fines Helvetiorum et "
                     "Allobrogum, qui nuper pacati erant, Rhodanus fluit isque non nullis locis vado transitur. Extremum oppidum Allobrogum est proximumque Helvetiorum finibus Genava. Ex eo oppido "
                     "pons ad Helvetios pertinet. Allobrogibus sese vel persuasuros, quod nondum bono animo in populum Romanum viderentur, existimabant vel vi coacturos ut per suos fines eos ire "
                     "paterentur. Omnibus rebus ad profectionem comparatis diem dicunt, qua die ad ripam Rhodani omnes conveniant. Is dies erat a. d. V. Kal. Apr. L. Pisone, A. Gabinio consulibus.\n"
                     "Caesari cum id nuntiatum esset, eos per provincia nostram iter facere conari, maturat ab urbe proficisci et quam maximis potest itineribus in Galliam ulteriorem contendit et ad "
                     "Genavam pervenit. Provinciae toti quam maximum potest militum numerum imperat (erat omnino in Gallia ulteriore legio una), pontem, qui erat ad Genavam, iubet rescindi. Ubi de eius "
                     "aventu Helvetii certiores facti sunt, legatos ad eum mittunt nobilissimos civitatis, cuius legationis Nammeius et Verucloetius principem locum obtinebant, qui dicerent sibi esse "
                     "in animo sine ullo maleficio iter per provinciam facere, propterea quod aliud iter haberent nullum: rogare ut eius voluntate id sibi facere liceat. Caesar, quod memoria tenebat "
                     "L. Cassium consulem occisum exercitumque eius ab Helvetiis pulsum et sub iugum missum, concedendum non putabat; neque homines inimico animo, data facultate per provinciam itineris "
                     "faciundi, temperaturos ab iniuria et maleficio existimabat. Tamen, ut spatium intercedere posset dum milites quos imperaverat convenirent, legatis respondit diem se ad "
                     "deliberandum sumpturum: si quid vellent, ad Id. April. reverterentur.\n"
                     "Interea ea legione quam secum habebat militibusque, qui ex provincia convenerant, a lacu Lemanno, qui in flumen Rhodanum influit, ad montem Iuram, qui fines Sequanorum ab "
                     "Helvetiis dividit, milia passuum XVIIII murum in altitudinem pedum sedecim fossamque perducit. Eo opere perfecto praesidia disponit, castella communit, quo facilius, si se invito "
                     "transire conentur, prohibere possit. Ubi ea dies quam constituerat cum legatis venit et legati ad eum reverterunt, negat se more et exemplo populi Romani posse iter ulli per "
                     "provinciam dare et, si vim lacere conentur, prohibiturum ostendit. Helvetii ea spe deiecti navibus iunctis ratibusque compluribus factis, alii vadis Rhodani, qua minima altitudo "
                     "fluminis erat, non numquam interdiu, saepius noctu si perrumpere possent conati, operis munitione et militum concursu et telis repulsi, hoc conatu destiterunt.\n"
                     "Relinquebatur una per Sequanos via, qua Sequanis invitis propter angustias ire non poterant. His cum sua sponte persuadere non possent, legatos ad Dumnorigem Haeduum mittunt, ut "
                     "eo deprecatore a Sequanis impetrarent. Dumnorix gratia et largitione apud Sequanos plurimum poterat et Helvetiis erat amicus, quod ex ea civitate Orgetorigis filiam in matrimonium "
                     "duxerat, et cupiditate regni adductus novis rebus studebat et quam plurimas civitates suo beneficio habere obstrictas volebat. Itaque rem suscipit et a Sequanis impetrat ut per "
                     "fines suos Helvetios ire patiantur, obsidesque uti inter sese dent perficit: Sequani, ne itinere Helvetios prohibeant, Helvetii, ut sine maleficio et iniuria transeant.\n"
                     "Caesari renuntiatur Helvetiis esse in animo per agrum Sequanorum et Haeduorum iter in Santonum fines facere, qui non longe a Tolosatium finibus absunt, quae civitas est in "
                     "provincia. Id si fieret, intellegebat magno cum periculo provinciae futurum ut homines bellicosos, populi Romani inimicos, locis patentibus maximeque frumentariis finitimos "
                     "haberet. Ob eas causas ei munitioni quam fecerat T. Labienum legatum praeficit; ipse in Italiam magnis itineribus contendit duasque ibi legiones conscribit et tres, quae circum "
                     "Aquileiam hiemabant, ex hibernis educit et, qua proximum iter in ulteriorem Galliam per Alpes erat, cum his quinque legionibus ire contendit. Ibi Ceutrones et Graioceli et "
                     "Caturiges locis superioribus occupatis itinere exercitum prohibere conantur. Compluribus his proeliis pulsis ab Ocelo, quod est oppidum citerioris provinciae extremum, in fines "
                     "Vocontiorum ulterioris provinciae die septimo pervenit; inde in Allobrogum fines, ab Allobrogibus in Segusiavos exercitum ducit. Hi sunt extra provinciam trans Rhodanum primi.";

std::string caesar_search_string = "Gallia";

// Here is a more complete stress test
std::string repeated(100000, 'A');
std::string repeated_search_string = "AAA";

// Test functor that lists all indices in order
struct kmp_functor
{
    void operator()(const size_t& index)
    {
        indices.push_back(index);
    }

    void merge(kmp_functor&& other)
    {
        indices.splice(indices.end(), std::move(other.indices));
    }

    // We use a list for more efficient splicing here.
    std::list<size_t> indices;
};

// Inefficient and naive reference implementation of string searching.
std::list<size_t> string_search(const std::string& haystack, const std::string& needle)
{
    std::list<size_t> indices;
    for (size_t index = 0; index < haystack.size() - needle.size() + 1; ++index)
    {
        std::string substring = haystack.substr(index, needle.size());
        if (substring == needle)
            indices.push_back(index);
    }
    return indices;
}

// Use a similar technique to parallel_kmp to enable us to easily test all 16 versions.
#define KMP_IMPL(...) \
    kmp_functor functor; \
    do { \
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<boost::asynchronous::lockfree_queue<>>>(6); \
        std::future<kmp_functor> fu = boost::asynchronous::post_future( \
            scheduler, \
            [] { return boost::asynchronous::parallel_kmp(__VA_ARGS__, kmp_functor {}, 1500, "parallel_kmp", 0); }, \
            "test_parallel_kmp", \
            0 \
        ); \
        try \
        { \
            functor = fu.get(); \
        } \
        catch (...) \
        { \
            BOOST_FAIL("Unexpected exception."); \
        } \
    } while (0)

#define AS_ITER(arg) arg.cbegin(), arg.cend()
#define AS_RNGE(arg) arg
#define AS_MOVE(arg) std::move(typename std::decay<decltype(arg)>::type(arg))
#define AS_CONT(arg) boost::asynchronous::then(boost::asynchronous::parallel_for(std::vector<int>(), [](int){}, 1, "dummy task", 0), [](boost::asynchronous::expected<std::vector<int>> exp) { exp.get(); return arg; })

#define TEST_RUN(haystack, needle, modif_a, modif_b) \
    do { \
        auto haystack_copy = haystack; \
        auto reference = string_search(haystack_copy, needle); \
        KMP_IMPL(AS_##modif_a(haystack), AS_##modif_b(needle)); \
        BOOST_CHECK_MESSAGE(functor.indices.size() != 0, "Needle not found in " #haystack); \
        BOOST_CHECK_MESSAGE(functor.indices == reference, "Result differs from reference implementation for " #haystack); \
        for (size_t index : functor.indices) \
            BOOST_CHECK_MESSAGE(haystack_copy.substr(index, needle.size()) == needle, "Got index without match in " #haystack ": " + std::to_string(index)); \
    } while(0)

#define SINGLE_TEST(name, haystack, needle, modif_a, modif_b) \
    BOOST_AUTO_TEST_CASE(name) \
    { \
        TEST_RUN(haystack, needle, modif_a, modif_b); \
    }

// Unfortunately, building all the different versions simultaneously also requires way too much RAM.
// So, we just test a couple combinations to ensure that each element has been tested in each position.
SINGLE_TEST(test_parallel_kmp_caesar_iter_iter, caesar, caesar_search_string, ITER, ITER)
SINGLE_TEST(test_parallel_kmp_caesar_rnge_rnge, caesar, caesar_search_string, RNGE, RNGE)
SINGLE_TEST(test_parallel_kmp_caesar_move_move, caesar, caesar_search_string, MOVE, MOVE)
SINGLE_TEST(test_parallel_kmp_caesar_cont_cont, caesar, caesar_search_string, CONT, CONT)

SINGLE_TEST(test_parallel_kmp_aaaaaa_iter_move, repeated, repeated_search_string, ITER, MOVE)
SINGLE_TEST(test_parallel_kmp_aaaaaa_cont_iter, repeated, repeated_search_string, CONT, ITER)
SINGLE_TEST(test_parallel_kmp_aaaaaa_rnge_cont, repeated, repeated_search_string, RNGE, CONT)

}
