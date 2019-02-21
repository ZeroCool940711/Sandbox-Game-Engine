commit a15d86bd6df19e7ec4dc167feefdfeaaa839275a
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Fri Feb 15 07:28:28 2019 -0700

    Re-Applying commit 24242127c226e65838062e4babca03972a387daf after merging some old stuff that was not committed before and it broke a few things, now everythign should be back as normal.

 src/lib/third_party/stack_walker/StackWalker.cpp   | 1180 ++++++++++++--------
 src/lib/third_party/stack_walker/StackWalker.h     |  256 +++--
 .../third_party/stack_walker/StackWalker_VC70.sln  |   29 +-
 .../stack_walker/StackWalker_VC70.vcproj           |   74 +-
 .../third_party/stack_walker/StackWalker_VC71.sln  |   29 +-
 .../stack_walker/StackWalker_VC71.vcproj           |   72 +-
 .../third_party/stack_walker/StackWalker_VC8.sln   |   27 +-
 .../stack_walker/StackWalker_VC8.vcproj            |  182 +--
 .../third_party/stack_walker/StackWalker_VC9.suo   |  Bin 9216 -> 9216 bytes
 src/lib/third_party/stack_walker/main.cpp          |  316 ++++--
 src/lib/third_party/stack_walker/makefile          |    4 +-
 11 files changed, 1272 insertions(+), 897 deletions(-)

commit 24242127c226e65838062e4babca03972a387daf
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Fri Feb 15 06:46:43 2019 -0700

    -Added new version of StackWalker. This version was last updated on August 2018 while the previous version the engine had was updated on 2005.
    - Added some Visual Studio solutions for different Visual Studio versions (VC5-VC2017).

 src/lib/third_party/stack_walker/StackWalker.cpp   | 1180 ++++++++++++--------
 src/lib/third_party/stack_walker/StackWalker.h     |  256 +++--
 .../third_party/stack_walker/StackWalker_VC70.sln  |   29 +-
 .../stack_walker/StackWalker_VC70.vcproj           |   74 +-
 .../third_party/stack_walker/StackWalker_VC71.sln  |   29 +-
 .../stack_walker/StackWalker_VC71.vcproj           |   72 +-
 .../third_party/stack_walker/StackWalker_VC8.sln   |   27 +-
 .../stack_walker/StackWalker_VC8.vcproj            |  182 +--
 src/lib/third_party/stack_walker/main.cpp          |  316 ++++--
 src/lib/third_party/stack_walker/makefile          |    4 +-
 10 files changed, 1272 insertions(+), 897 deletions(-)

commit b74dc76a095c2c07a62f6f16378a6fca610a90c7
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Tue Feb 12 04:17:15 2019 -0700

    Updated the changelog file.

 changelog.md | 37 +++++++++++++++++++++++++++++++++++++
 1 file changed, 37 insertions(+)

commit 374e45edcc8787822af79a9dc26c8c90f2eb4bd8
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Tue Feb 12 04:12:51 2019 -0700

    -Disabled a Speedtree Error Message for the composite map or atlas map that was unnecessary and was causing performance issues. This change gave us a boost of up to 20 fps when using old speedtree spt models.

 src/lib/speedtree/speedtree_renderer_util.cpp | 28 +++++++++++++++------------
 1 file changed, 16 insertions(+), 12 deletions(-)

commit 623f4b67f459e2670d36e591942cfe42f0f824b8
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Tue Feb 12 03:40:43 2019 -0700

    -Changing the Max Texture Size on the texture_aggregator.cpp, previously it had a value of 2048 max which was causing some problems on trees with big texture atlas.
    - This change allow now to use textures up to 8K resolution.

 src/lib/moo/texture_aggregator.cpp | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)

commit 730b47030d50bfcd7d388437501d8514b7388f23
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Tue Feb 12 03:38:09 2019 -0700

    Re-applying commit 37b6e596, for some reason it was reverted on commit b42df620.

 bigworld/tools/worldeditor/options.xml | 12 ++++++------
 1 file changed, 6 insertions(+), 6 deletions(-)

commit abae340d704693150c6133c15e7fc2c4f5b84f12
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Tue Feb 12 03:35:40 2019 -0700

    -Added a Changelog.md file that is created from commits for easy understanding what has changed over time.

 changelog.md | 60 +++++++++++++++++++++++++++++++++++++++++++++++-------------
 1 file changed, 47 insertions(+), 13 deletions(-)

commit a01829eb306c2bb76d4b312638c6ebc256637c6c
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Tue Feb 12 03:25:08 2019 -0700

    Continuing to remove unnecessary files from the repository and adding them to the gitignore file.

 src/tools/worldeditor/Editor_Hybrid_vc2005/.gitignore | 2 ++
 1 file changed, 2 insertions(+)

commit b42df620b00bdbc033bde3d5de2a32a66470e51b
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Mon Feb 11 23:54:59 2019 -0700

    Revert "Changed the bspBoundingBox option to a value of 0 on the options.xml file for the worldeditor, this will allow us to select objects by selecting their BSP or mesh instead of their bounding box which is better when working on scenes with many objects."
    
    This reverts commit 37b6e59648f616e3cbe632b119c41d845403e8e8.

 bigworld/tools/worldeditor/.gitignore | 1 -
 1 file changed, 1 deletion(-)

commit b39464f4195b06865b72119066e370d801e11e88
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Mon Feb 11 23:53:07 2019 -0700

    Changed the max strength value for the mask when painting using a heightmap mask, this will give more control when importing a mask for painting, some times the mask used need more strength as it wasn't correctly created or imported.

 src/tools/worldeditor/gui/pages/page_terrain_texture.cpp | 4 ++--
 1 file changed, 2 insertions(+), 2 deletions(-)

commit ced94d6af5d2b8798c5f647f6883fecb0037a035
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Mon Feb 11 23:47:15 2019 -0700

    Changed the terrain max value for the Create New Space window, this allow us to create terrains with a max size of 1000000Km now, keep in mind that creating a terrain of this size will take a long time to generate, we need to find ways to optimize this generation as it is quite slow right now even for small maps.

 src/tools/worldeditor/gui/dialogs/new_space_dlg.cpp | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)

commit 019d68eca1795745a1bb3f0a5ae2aa0942f17bf3
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Mon Feb 11 23:43:56 2019 -0700

    Adding the BuildLog.html files to the ignore list as there is no reason to upload them to the repository.

 bigworld/tools/worldeditor/options.xml     |   2 +-
 bigworld/tools/worldeditor/worldeditor.exe | Bin 10783232 -> 10786304 bytes
 2 files changed, 1 insertion(+), 1 deletion(-)

commit bab37e0736101d2b4a96177c1daff47ec1f44528
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Mon Feb 11 23:53:07 2019 -0700

    Changed the max strength value for the mask when painting using a heightmap mask, this will give more control when importing a mask for painting, some times the mask used need more strength as it wasn't correctly created or imported.

 src/tools/worldeditor/gui/pages/page_terrain_texture.cpp | 4 ++--
 1 file changed, 2 insertions(+), 2 deletions(-)

commit 10646d447b5af003168523f77e8d7b7022eb93ee
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Mon Feb 11 23:47:15 2019 -0700

    Changed the terrain max value for the Create New Space window, this allow us to create terrains with a max size of 1000000Km now, keep in mind that creating a terrain of this size will take a long time to generate, we need to find ways to optimize this generation as it is quite slow right now even for small maps.

 src/tools/worldeditor/gui/dialogs/new_space_dlg.cpp | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)

commit 508db15ad64a0c72cd2e63e927985aa617d313a2
Author: ZeroCool <alejandrogilelias940711@gmail.com>
Date:   Mon Feb 11 23:43:56 2019 -0700

    Adding the BuildLog.html files to the ignore list as there is no reason to upload them to the repository.

 src/lib/camera/Hybrid_vc2005/.gitignore                 | 1 +
 src/lib/chunk/Hybrid_vc2005/.gitignore                  | 1 +
 src/lib/cstdmf/unit_test/Hybrid_vc2005/.gitignore       | 1 +
 src/lib/fmodsound/Hybrid_vc2005/.gitignore              | 1 +
 src/lib/math/unit_test/Hybrid_vc2005/.gitignore         | 1 +
 src/lib/network/unit_test/Hybrid_vc2005/.gitignore      | 1 +
 src/lib/physics2/unit_test/Hybrid_vc2005/.gitignore     | 1 +
 src/lib/png/projects/visualc71/Hybrid_vc2005/.gitignore | 1 +
 src/lib/resmgr/unit_test/Hybrid_vc2005/.gitignore       | 1 +
 src/lib/speedtree/Hybrid_vc2005/.gitignore              | 1 +
 src/lib/terrain/unit_test/Hybrid_vc2005/.gitignore      | 1 +
 11 files changed, 11 insertions(+)
