
#include "StringUtil.h"
#include "StringTokenizer.h"
#include "fs.h"

#include "msw-app-console-init.h"
#include "StringUtil.h"

static const char* parse_inputs[] = {
    "",
    "--lvalue=rvalue1",
    "--lvalue=rvalue1,revalue2",
    "--lvalue=rvalue1,revalue2,three",
    "--lvalue = rvalue1, rvalue2",
    "--lvalue\t=\trvalue1,\trevalue2",
    "--lvalue=",
    "--lvalue",
    " --lvalue = はい。 , はい。, おはようございます",
    " --値 = はい。 , はい。, おはようございます",
    nullptr,
};

#if 0
static const char* parse_multiline_inputs[] = {
  "LVALUE = RVALUE\n"
  "LVALUE2 = RVALUE(DOS)\r\n"
  "LINE3 = RVALUE(multiline)\n\n"
  "LINE4 = FINISHED\n"
};
#endif

// note: verification is done by bash script externally.
/* Expected output, use diff CLI too to verify:
--lvalue
--lvalue = rvalue1
--lvalue = rvalue1,revalue2
--lvalue = rvalue1,revalue2,three
--lvalue = rvalue1,rvalue2
--lvalue = rvalue1,revalue2
--lvalue

--lvalue = はい。,はい。,おはようございます
--値 = はい。,はい。,おはようございます

*/

static const char* path_abs_inputs[] = {
    "/c/"                               ,
    "/c/one"                            ,
    "/c/one"                            ,
    "/c/one\\two\\three"                ,
    "/c/one\\..\\one\\two\\three"       ,
    "/c/one\\../one/two\\three"         ,
    "/c/one/two\\three"                 ,
    "/dev/null"                         ,

    "c"                                 ,
    "c:"                                ,
    "c:\\"                              ,
    "c:/"                               ,
    "c:\\one"                           ,
    "c:\\one\\"                         ,
    "c:/one"                            ,
    "c:/one/"                           ,
    "c:\\one two"                       ,
    "c:\\one\\two\\three"               ,
    "c:\\one two\\three"                ,
    "c:\\one\\..\\one\\two\\three"      ,
    "c:\\one\\../one/two\\three"        ,
    "c:/one/two\\three"                 ,
};

static const char* path_rel_inputs[] = {
    "ex"                        ,
    "./ex"                      ,
    "./ex/"                     ,
    "../ex"                     ,
    "ex/why"                    ,
    "ex/why/"                   ,
    "./ex/!why/"                ,
    "../ex/!why/zee"            ,
    "ex !why/zee"               ,
    "./ex !why/zee"             ,
    "./ex why/zee"              ,
};

// for confirming behavior on filesystems that natively support mount paths longer than one character.
static const char* path_absmount_inputs[] = {
    "/rom/"                             ,
    "/rom/one"                          ,
    "/rom/one"                          ,
    "/rom/one\\two\\three"              ,
    "/root/one\\..\\one\\two\\three"    ,
    "/root/one\\../one/two\\three"      ,
    "/root/one/two\\three"              ,
    "/dev/null"                         ,

    "rom"                                 ,
    "rom:"                                ,
    "rom:\\"                              ,
    "rom:/"                               ,
    "rom:\\one"                           ,
    "rom:\\one\\"                         ,
    "rom:/one"                            ,
    "rom:/one/"                           ,
    "rom:\\one two"                       ,
    "rom:\\one\\two\\three"               ,
    "rom:\\one two\\three"                ,
    "rom:\\one\\..\\one\\two\\three"      ,
    "rom:\\one\\../one/two\\three"        ,
    "rom:/one/two\\three"                 ,
};

int main(int argc, char** argv) {

	msw_AllocConsoleForWindowedApp();

    if (argc > 1 && strcasecmp(argv[1], "crashdump") == 0) {
        *((int volatile*)0) = 50;
        exit(0);
    }

    printf("TEST:TOKENIZER:SINGLINE\n");
    for(const auto* item : parse_inputs) {
        auto tok = Tokenizer(item);
        if (auto* lvalue = tok.GetNextToken('=')) {
            printf("%s", lvalue);
            bool need_comma = 0;
            while(auto* rparam = tok.GetNextToken(',')) {
                printf("%s%s", need_comma ? "," : "=", rparam);
                need_comma = 1;
            }
        }
        printf("\n");
    }

#if 0
    printf("--------------------------------------\n");
    printf("TEST:TOKENIZER:MULTILINE\n");
    auto tokall = Tokenizer(parse_multiline_inputs[0]);
    while(auto line = tokall.GetNextToken("\r\n")) {
        auto tok = Tokenizer(line);
        if (auto* lvalue = tok.GetNextToken('=')) {
            printf("%s", lvalue);
            bool need_comma = 0;
            while(auto* rparam = tok.GetNextToken(',')) {
                printf("%s%s", need_comma ? "," : "=", rparam);
                need_comma = 1;
            }
            printf("\n");
        }
    }
#endif

    printf("--------------------------------------\n");
    printf("TEST:FILESYSTEM:ABSOLUTE\n");
    for(const auto* item : path_abs_inputs) {
        printf("input  = %s\n", item);
        auto abs_part = (fs::path)item;
        auto msw_path = fs::ConvertToMsw(item);
        printf("uni    = %s\n", abs_part.uni_string().c_str());
        printf("msw    = %s\n", msw_path.c_str());
        printf("native = %s\n", abs_part.c_str());
        printf("\n");
    }
    printf("--------------------------------------\n");
    printf("TEST:FILESYSTEM:ABSOLUTE_CONCAT_ABS\n");
    for(const auto* itemA : path_abs_inputs) {
        for(const auto* itemB : path_abs_inputs) {
            printf("inputA = %s\n", itemA);
            printf("inputB = %s\n", itemB);
            auto concatenated = (fs::path)itemA / itemB;
            printf("uni    = %s\n", concatenated.uni_string().c_str());
            printf("native = %s\n", concatenated.c_str());
            printf("\n");
        }
    }
    printf("--------------------------------------\n");
    printf("TEST:FILESYSTEM:ABSOLUTE_CONCAT_REL\n");
    for(const auto* itemA : path_abs_inputs) {
        for(const auto* itemB : path_rel_inputs) {
            printf("inputA = %s\n", itemA);
            printf("inputB = %s\n", itemB);
            auto concatenated = (fs::path)itemA / itemB;
            printf("uni    = %s\n", concatenated.uni_string().c_str());
            printf("native = %s\n", concatenated.c_str());
            printf("\n");
        }
    }

    printf("--------------------------------------\n");
    printf("TEST:FILESYSTEM:REL_CONCAT_ABS\n");
    for(const auto* itemA : path_rel_inputs) {
        for(const auto* itemB : path_abs_inputs) {
            printf("inputA = %s\n", itemA);
            printf("inputB = %s\n", itemB);
            auto concatenated = (fs::path)itemA / itemB;
            printf("uni    = %s\n", concatenated.uni_string().c_str());
            printf("native = %s\n", concatenated.c_str());
            printf("\n");
        }
    }
    printf("--------------------------------------\n");
    printf("TEST:FILESYSTEM:REL_CONCAT_REL\n");
    for(const auto* itemA : path_rel_inputs) {
        for(const auto* itemB : path_rel_inputs) {
            printf("inputA = %s\n", itemA);
            printf("inputB = %s\n", itemB);
            auto concatenated = (fs::path)itemA / itemB;
            printf("uni    = %s\n", concatenated.uni_string().c_str());
            printf("native = %s\n", concatenated.c_str());
            printf("\n");
        }
    }

	//__debugbreak();
    printf("--------------------------------------\n");
    printf("TEST:FILESYSTEM:ConvertFromMsw(mountLength=15)\n");
    for(const auto* itemA : path_absmount_inputs) {
        printf("input  = %s\n", itemA);
		auto unipath = fs::PathFromString(itemA, 15);
		//if (unipath == "/rom") __debugbreak();
		auto native  = fs::ConvertToMswMixed(unipath.c_str(), 15);
		printf("uni    = %s\n", unipath.c_str());
        printf("native = %s\n", native.c_str());
        printf("\n");
    }

    printf("--------------------------------------\n");
    printf("END OF TEST LOG\n");

    return 0;
}
