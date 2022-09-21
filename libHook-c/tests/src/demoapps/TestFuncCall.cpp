#include <cstdio>
#include <FuncWithDiffParms.h>
#include <CallFunctionCall.h>
#include <TenThousandFunc.h>
#include <sys/prctl.h>
#include <thread>
#include <cassert>
#include <iostream>
using namespace std;


inline int64_t getunixtimestampms() {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((int64_t) hi << 32) | lo;
}

struct A123 {
    A123(int &&n) { std::cout << "rvalue overload, n=" << n << "\n"; }

    A123(int &n) { std::cout << "lvalue overload, n=" << n << "\n"; }
};

class B123 {
public:
    template<class T1, class T2, class T3>
    B123(T1 &&t1, T2 &&t2, T3 &&t3) :
            a1_{std::forward<T1>(t1)},
            a2_{std::forward<T2>(t2)},
            a3_{std::forward<T3>(t3)} {
    }

private:
    A123 a1_, a2_, a3_;
};

template<class T, class U>
std::unique_ptr<T> make_unique1(U &&u) {
    return std::unique_ptr<T>(new T(std::forward<U>(u)));
}

template<class T, class... U>
std::unique_ptr<T> make_unique2(U &&... u) {
    return std::unique_ptr<T>(new T(std::forward<U>(u)...));
}

void forwardTest() {
//    auto p1 = make_unique1<A123>(2); // rvalue
//    int i = 1;
//    auto p2 = make_unique1<A123>(i); // lvalue
//
//    std::cout << "B\n";
    auto t = make_unique2<B123>(2, 0, 3);
}

void *asmSize = (void *) forwardTest;

class ClassA {
public:
    void *a = nullptr;

    ClassA() {
        a = malloc(1);
    }

    ~ClassA() {
        free(a);
        a = nullptr;
    }
};

class ClassB {
public:
    int *b = nullptr;

    ClassB() {
        b = new int[1];
    }

    ~ClassB() {
        delete[] b;
    }
};

int main() {
    funcA();
    funcA();
    callFuncA();
    printf("Calling funcA\n");


    auto actualStart = getunixtimestampms();
    pthread_t pt1 = pthread_self();
    assert(pt1 != -1);
    printf("pt1=%lu\n", pt1);
    printf("pt1=%lu\n", myGetThreadID());
//
    printf("Calling funcA\n");
    funcA();
    funcA();
    funcA();

    callFuncA();


    printf("Calling funcB\n");
    funcB(1);

    printf("Calling funcC\n");
    funcC(1, 2);

    printf("Calling funcD\n");
    funcD(1, 2, 3);

    printf("Calling funcE\n");
    funcE(1, 2, 3);

    printf("Calling callFuncA\n");
    callFuncA();

    printf("Calling callFunc1000\n");
    callFunc1000();
    int a[] = {1, 2, 3, 4, 5};
    printf("a[]={1,2,3,4,5} starts at %p\n", a);

    printf("My id is: %lu\n", pthread_self());
//
    int rlt = system("ls -al");
    prctl(PR_SET_DUMPABLE, 1);
    {
        ClassA a;
    }
    {
        ClassB b;
    }



//
//    while (1) {
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//    }

    /*func1();
    func2();
    func3();
    func4();
    func5();
    func6();
    func7();
    func8();
    func9();
    func10();
    func11();
    func12();
    func13();
    func14();
    func15();
    func16();
    func17();
    func18();
    func19();
    func20();
    func21();
    func22();
    func23();
    func24();
    func25();
    func26();
    func27();
    func28();
    func29();
    func30();
    func31();
    func32();
    func33();
    func34();
    func35();
    func36();
    func37();
    func38();
    func39();
    func40();
    func41();
    func42();
    func43();
    func44();
    func45();
    func46();
    func47();
    func48();
    func49();
    func50();
    func51();
    func52();
    func53();
    func54();
    func55();
    func56();
    func57();
    func58();
    func59();
    func60();
    func61();
    func62();
    func63();
    func64();
    func65();
    func66();
    func67();
    func68();
    func69();
    func70();
    func71();
    func72();
    func73();
    func74();
    func75();
    func76();
    func77();
    func78();
    func79();
    func80();
    func81();
    func82();
    func83();
    func84();
    func85();
    func86();
    func87();
    func88();
    func89();
    func90();
    func91();
    func92();
    func93();
    func94();
    func95();
    func96();
    func97();
    func98();
    func99();
    func100();
    func101();
    func102();
    func103();
    func104();
    func105();
    func106();
    func107();
    func108();
    func109();
    func110();
    func111();
    func112();
    func113();
    func114();
    func115();
    func116();
    func117();
    func118();
    func119();
    func120();
    func121();
    func122();
    func123();
    func124();
    func125();
    func126();
    func127();
    func128();
    func129();
    func130();
    func131();
    func132();
    func133();
    func134();
    func135();
    func136();
    func137();
    func138();
    func139();
    func140();
    func141();
    func142();
    func143();
    func144();
    func145();
    func146();
    func147();
    func148();
    func149();
    func150();
    func151();
    func152();
    func153();
    func154();
    func155();
    func156();
    func157();
    func158();
    func159();
    func160();
    func161();
    func162();
    func163();
    func164();
    func165();
    func166();
    func167();
    func168();
    func169();
    func170();
    func171();
    func172();
    func173();
    func174();
    func175();
    func176();
    func177();
    func178();
    func179();
    func180();
    func181();
    func182();
    func183();
    func184();
    func185();
    func186();
    func187();
    func188();
    func189();
    func190();
    func191();
    func192();
    func193();
    func194();
    func195();
    func196();
    func197();
    func198();
    func199();
    func200();
    func201();
    func202();
    func203();
    func204();
    func205();
    func206();
    func207();
    func208();
    func209();
    func210();
    func211();
    func212();
    func213();
    func214();
    func215();
    func216();
    func217();
    func218();
    func219();
    func220();
    func221();
    func222();
    func223();
    func224();
    func225();
    func226();
    func227();
    func228();
    func229();
    func230();
    func231();
    func232();
    func233();
    func234();
    func235();
    func236();
    func237();
    func238();
    func239();
    func240();
    func241();
    func242();
    func243();
    func244();
    func245();
    func246();
    func247();
    func248();
    func249();
    func250();
    func251();
    func252();
    func253();
    func254();
    func255();
    func256();
    func257();
    func258();
    func259();
    func260();
    func261();
    func262();
    func263();
    func264();
    func265();
    func266();
    func267();
    func268();
    func269();
    func270();
    func271();
    func272();
    func273();
    func274();
    func275();
    func276();
    func277();
    func278();
    func279();
    func280();
    func281();
    func282();
    func283();
    func284();
    func285();
    func286();
    func287();
    func288();
    func289();
    func290();
    func291();
    func292();
    func293();
    func294();
    func295();
    func296();
    func297();
    func298();
    func299();
    func300();
    func301();
    func302();
    func303();
    func304();
    func305();
    func306();
    func307();
    func308();
    func309();
    func310();
    func311();
    func312();
    func313();
    func314();
    func315();
    func316();
    func317();
    func318();
    func319();
    func320();
    func321();
    func322();
    func323();
    func324();
    func325();
    func326();
    func327();
    func328();
    func329();
    func330();
    func331();
    func332();
    func333();
    func334();
    func335();
    func336();
    func337();
    func338();
    func339();
    func340();
    func341();
    func342();
    func343();
    func344();
    func345();
    func346();
    func347();
    func348();
    func349();
    func350();
    func351();
    func352();
    func353();
    func354();
    func355();
    func356();
    func357();
    func358();
    func359();
    func360();
    func361();
    func362();
    func363();
    func364();
    func365();
    func366();
    func367();
    func368();
    func369();
    func370();
    func371();
    func372();
    func373();
    func374();
    func375();
    func376();
    func377();
    func378();
    func379();
    func380();
    func381();
    func382();
    func383();
    func384();
    func385();
    func386();
    func387();
    func388();
    func389();
    func390();
    func391();
    func392();
    func393();
    func394();
    func395();
    func396();
    func397();
    func398();
    func399();
    func400();
    func401();
    func402();
    func403();
    func404();
    func405();
    func406();
    func407();
    func408();
    func409();
    func410();
    func411();
    func412();
    func413();
    func414();
    func415();
    func416();
    func417();
    func418();
    func419();
    func420();
    func421();
    func422();
    func423();
    func424();
    func425();
    func426();
    func427();
    func428();
    func429();
    func430();
    func431();
    func432();
    func433();
    func434();
    func435();
    func436();
    func437();
    func438();
    func439();
    func440();
    func441();
    func442();
    func443();
    func444();
    func445();
    func446();
    func447();
    func448();
    func449();
    func450();
    func451();
    func452();
    func453();
    func454();
    func455();
    func456();
    func457();
    func458();
    func459();
    func460();
    func461();
    func462();
    func463();
    func464();
    func465();
    func466();
    func467();
    func468();
    func469();
    func470();
    func471();
    func472();
    func473();
    func474();
    func475();
    func476();
    func477();
    func478();
    func479();
    func480();
    func481();
    func482();
    func483();
    func484();
    func485();
    func486();
    func487();
    func488();
    func489();
    func490();
    func491();
    func492();
    func493();
    func494();
    func495();
    func496();
    func497();
    func498();
    func499();
    func500();
    func501();
    func502();
    func503();
    func504();
    func505();
    func506();
    func507();
    func508();
    func509();
    func510();
    func511();
    func512();
    func513();
    func514();
    func515();
    func516();
    func517();
    func518();
    func519();
    func520();
    func521();
    func522();
    func523();
    func524();
    func525();
    func526();
    func527();
    func528();
    func529();
    func530();
    func531();
    func532();
    func533();
    func534();
    func535();
    func536();
    func537();
    func538();
    func539();
    func540();
    func541();
    func542();
    func543();
    func544();
    func545();
    func546();
    func547();
    func548();
    func549();
    func550();
    func551();
    func552();
    func553();
    func554();
    func555();
    func556();
    func557();
    func558();
    func559();
    func560();
    func561();
    func562();
    func563();
    func564();
    func565();
    func566();
    func567();
    func568();
    func569();
    func570();
    func571();
    func572();
    func573();
    func574();
    func575();
    func576();
    func577();
    func578();
    func579();
    func580();
    func581();
    func582();
    func583();
    func584();
    func585();
    func586();
    func587();
    func588();
    func589();
    func590();
    func591();
    func592();
    func593();
    func594();
    func595();
    func596();
    func597();
    func598();
    func599();
    func600();
    func601();
    func602();
    func603();
    func604();
    func605();
    func606();
    func607();
    func608();
    func609();
    func610();
    func611();
    func612();
    func613();
    func614();
    func615();
    func616();
    func617();
    func618();
    func619();
    func620();
    func621();
    func622();
    func623();
    func624();
    func625();
    func626();
    func627();
    func628();
    func629();
    func630();
    func631();
    func632();
    func633();
    func634();
    func635();
    func636();
    func637();
    func638();
    func639();
    func640();
    func641();
    func642();
    func643();
    func644();
    func645();
    func646();
    func647();
    func648();
    func649();
    func650();
    func651();
    func652();
    func653();
    func654();
    func655();
    func656();
    func657();
    func658();
    func659();
    func660();
    func661();
    func662();
    func663();
    func664();
    func665();
    func666();
    func667();
    func668();
    func669();
    func670();
    func671();
    func672();
    func673();
    func674();
    func675();
    func676();
    func677();
    func678();
    func679();
    func680();
    func681();
    func682();
    func683();
    func684();
    func685();
    func686();
    func687();
    func688();
    func689();
    func690();
    func691();
    func692();
    func693();
    func694();
    func695();
    func696();
    func697();
    func698();
    func699();
    func700();
    func701();
    func702();
    func703();
    func704();
    func705();
    func706();
    func707();
    func708();
    func709();
    func710();
    func711();
    func712();
    func713();
    func714();
    func715();
    func716();
    func717();
    func718();
    func719();
    func720();
    func721();
    func722();
    func723();
    func724();
    func725();
    func726();
    func727();
    func728();
    func729();
    func730();
    func731();
    func732();
    func733();
    func734();
    func735();
    func736();
    func737();
    func738();
    func739();
    func740();
    func741();
    func742();
    func743();
    func744();
    func745();
    func746();
    func747();
    func748();
    func749();
    func750();
    func751();
    func752();
    func753();
    func754();
    func755();
    func756();
    func757();
    func758();
    func759();
    func760();
    func761();
    func762();
    func763();
    func764();
    func765();
    func766();
    func767();
    func768();
    func769();
    func770();
    func771();
    func772();
    func773();
    func774();
    func775();
    func776();
    func777();
    func778();
    func779();
    func780();
    func781();
    func782();
    func783();
    func784();
    func785();
    func786();
    func787();
    func788();
    func789();
    func790();
    func791();
    func792();
    func793();
    func794();
    func795();
    func796();
    func797();
    func798();
    func799();
    func800();
    func801();
    func802();
    func803();
    func804();
    func805();
    func806();
    func807();
    func808();
    func809();
    func810();
    func811();
    func812();
    func813();
    func814();
    func815();
    func816();
    func817();
    func818();
    func819();
    func820();
    func821();
    func822();
    func823();
    func824();
    func825();
    func826();
    func827();
    func828();
    func829();
    func830();
    func831();
    func832();
    func833();
    func834();
    func835();
    func836();
    func837();
    func838();
    func839();
    func840();
    func841();
    func842();
    func843();
    func844();
    func845();
    func846();
    func847();
    func848();
    func849();
    func850();
    func851();
    func852();
    func853();
    func854();
    func855();
    func856();
    func857();
    func858();
    func859();
    func860();
    func861();
    func862();
    func863();
    func864();
    func865();
    func866();
    func867();
    func868();
    func869();
    func870();
    func871();
    func872();
    func873();
    func874();
    func875();
    func876();
    func877();
    func878();
    func879();
    func880();
    func881();
    func882();
    func883();
    func884();
    func885();
    func886();
    func887();
    func888();
    func889();
    func890();
    func891();
    func892();
    func893();
    func894();
    func895();
    func896();
    func897();
    func898();
    func899();
    func900();
    func901();
    func902();
    func903();
    func904();
    func905();
    func906();
    func907();
    func908();
    func909();
    func910();
    func911();
    func912();
    func913();
    func914();
    func915();
    func916();
    func917();
    func918();
    func919();
    func920();
    func921();
    func922();
    func923();
    func924();
    func925();
    func926();
    func927();
    func928();
    func929();
    func930();
    func931();
    func932();
    func933();
    func934();
    func935();
    func936();
    func937();
    func938();
    func939();
    func940();
    func941();
    func942();
    func943();
    func944();
    func945();
    func946();
    func947();
    func948();
    func949();
    func950();
    func951();
    func952();
    func953();
    func954();
    func955();
    func956();
    func957();
    func958();
    func959();
    func960();
    func961();
    func962();
    func963();
    func964();
    func965();
    func966();
    func967();
    func968();
    func969();
    func970();
    func971();
    func972();
    func973();
    func974();
    func975();
    func976();
    func977();
    func978();
    func979();
    func980();
    func981();
    func982();
    func983();
    func984();
    func985();
    func986();
    func987();
    func988();
    func989();
    func990();
    func991();
    func992();
    func993();
    func994();
    func995();
    func996();
    func997();
    func998();
    func999();*/

//    auto actualEnd = getunixtimestampms();
//    printf("%ld-%ld=%ld\n", actualEnd, actualStart, actualEnd - actualStart);

    forwardTest();
    return 0;
}