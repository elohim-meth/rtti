#include "test_dll.h"
#include "test_register.h"

void define_test_namespace(rtti::meta_define<void> define)
{
    define
        ._attribute("Description", "Rtti test namespace")
        ._enum<test::Color>("Color")
            ._element("Red", test::Color::Red)
            ._element("Green", test::Color::Green)
            ._element("Blue", test::Color::Blue)

        ._class<test::Test1>("Test1")
            ._attribute("Description", "Absolutly empty class")
        ._end()

        ._class<test::Test2>("Test2") // warning! no implicitly declared move and deprecated copy!
            ._attribute("Description", "Has only user delared virtual destructor defined as default")
        ._end()

        ._class<test::Test3>("Test3")
            ._attribute("Description", "Has user declared copy and move defined as default")
        ._end()

//This class is impossible to register cause variant requires CopyConstructible or MoveConstructible
//      ._class<test::Test4>("Test4")
//          ._attribute("Description", "Unmovable default constructible class")
//      ._end()

        ._class<test::Test5>("Test5") // warning! move participates in overload and not degenerate to copy!
            ._attribute("Description", "Has user declared copy defined as default and user declared move defined as delete")
        ._end()

        ._class<test::Test6>("Test6")
            ._attribute("Description", "Has user declared copy defined as default and not declared move")
        ._end()

        ._class<test::Test7>("Test7")
            ._attribute("Description", "Has user declared move defined as default and not declared copy")
        ._end()

//This class is impossible to register cause variant requires CopyConstructible or MoveConstructible
//      ._class<test::Test8>("Test8")
//          ._attribute("Description", "Has user declared move defined as delete and not declared copy")
//      ._end()
        ._class<test::CopyAndMove>("CopyAndMove")
        ._end()
        ._class<test::CopyOnly>("CopyOnly")
        ._end()
        ._class<test::MoveOnly>("MoveOnly")
        ._end()

    ;
}
