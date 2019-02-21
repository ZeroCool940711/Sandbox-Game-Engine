/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "stdafx.h"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/zip_section.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/multi_file_system.hpp"
#include "cstdmf/bwrandom.hpp"
#include "cstdmf/cstdmf.hpp"

#define TEST_PRINT_NAME() printf( "Running test: %s\n", m_name )

namespace
{
	/**
	 *	This compares two memory blocks to see if they are exactly the same.
	 *
	 *	@param block1	The address of the first block.
	 *	@param block1Sz	The size of the first block.
	 *	@param block2	The address of the second block.
	 *	@param block2Sz	The size of the second block.
	 *  @return			True if the blocks are the same, false if they differ.
	 */
	bool compMemBlocks
	(
		void		const *block1,
		size_t		block1Sz,
		void		const *block2,
		size_t		block2Sz
	)
	{
		if (block1Sz != block2Sz)
			return false;
		return ::memcmp(block1, block2, block1Sz) == 0;
	}


	/**
	 *	This converts an object to a string representation.  It assumes that
	 *	operator<< is defined.
	 */
	template<typename T>
	std::string toString(T const &t)
	{
		std::stringstream out;
		out << t;
		return out.str();
	}


	/**
	 *	This is the number of files to write/read in a lots-of-files test.
	 */
	const size_t NUM_LOTS_OF_FILES = 2000;
}

/**
 *	This tests checks that a non-existant zip section does not actually exist.
 */
TEST( ZipSection_NonExistent )
{
	DataSectionPtr emptySection =
		BWResource::openSection("flying_cats");
	CHECK(!emptySection);
}


/**
 *	This tests whether an existant section exists.
 */
TEST( ZipSection_Existent )
{
	DataSectionPtr zipSection = BWResource::openSection("exist.zip");
	CHECK(zipSection);
}


/**
 *	This tests whether reading an empty section can be read and whether it
 *	is empty.
 */
TEST( ZipSection_Empty )
{
	DataSectionPtr zipSection = BWResource::openSection("empty.zip");
	CHECK(zipSection);
	CHECK(zipSection->countChildren() == 0);
}


/**
 *	This opens an existing section and compares it with known values.
 */
TEST( ZipSection_ReadNumbers )
{
	DataSectionPtr zipSection = BWResource::openSection("numbers.zip");
	CHECK(zipSection);
	DataSectionPtr numSection = zipSection->openSection("child");
	CHECK(numSection);

	const int data[]    = { 4, 8, 15, 16, 23, 42 };
	BinaryPtr binData = numSection->asBinary();
	CHECK
	(
		compMemBlocks
		(
			&data[0]       , sizeof(data)  ,
			binData->data(), binData->len()
		)
	);
}


/**
 *	This tests writing and reading lots of files.
 */
TEST( ZipSection_LotsOfFiles )
{
	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();	
	for (size_t i = 0; i < NUM_LOTS_OF_FILES; ++i)
	{
		std::string filename = 
			"lots_of_files/test_file_" + toString((int)i) + ".zip";		

		fileSystem->eraseFileOrDirectory( filename );
	}	

	BWRandom				random;
	std::vector<BinaryPtr>	data;

	// Write out NUM_LOTS_OF_FILES files of random length between 512 and 1024
	// bytes with random data.
	for (size_t i = 0; i < NUM_LOTS_OF_FILES; ++i)
	{
		std::string filename =
			"lots_of_files/test_file_" + toString((int)i) + ".zip";
		DataSectionPtr zipSection = BWResource::openSection("test", true,
										ZipSection::creator());
		CHECK(zipSection);
		DataSectionPtr child = zipSection->openSection("child", true,
									BinSection::creator());
		CHECK(child);
		std::vector<uint8> buffer(random(512, 1024));
		if (buffer.size()>=2)
			buffer[0] = buffer[1] = 255;	
		for (size_t j = 2; j < buffer.size(); ++j)
		{
			uint8 val = (uint8)random( 0, 254 );
			if ( val == '<' )
				val = 255;
			buffer[j] = val;
		}
		BinaryPtr bin = new BinaryBlock(&buffer[0], buffer.size(), "unit_test");
		data.push_back(bin);
		bool ret = child->setBinary(bin);
		CHECK(ret);
		ret = zipSection->save(filename);
		CHECK(ret);
	}

	BWResource::instance().purgeAll();	// Clear any cached values

	// Read in the written files and compare the on-disk values with the values
	// in memory.
	for (size_t i = 0; i < NUM_LOTS_OF_FILES; ++i)
	{
		std::string filename =
			"lots_of_files/test_file_" + toString((int)i) + ".zip";
		DataSectionPtr zipSection = BWResource::openSection(filename);
		CHECK( zipSection.getObject() != NULL );
		DataSectionPtr numSection = zipSection->openSection("child", false,
										BinSection::creator());
		CHECK( numSection.getObject() != NULL );
		BinaryPtr binData = numSection->asBinary();
		CHECK(binData);
		CHECK
		(
			compMemBlocks
			(
				binData->data(), binData->len(),
				data[i]->data(), data[i]->len()
			)
		);
	}

	BWResource::instance().purgeAll();	// Clear any cached values
}

BinaryPtr randomBlock( BWRandom& random )
{
	std::vector<uint8> buffer( random(512, 1024) );
	for (size_t j = 0; j < buffer.size(); ++j)	
	{
		uint8 val = (uint8)random( 0, 254 );
		if ( val == '<' )
			val = 255;
		buffer[j] = val;
	}
	
	return new BinaryBlock( &buffer[0], buffer.size(), "unit_test" );
}

bool checkBlocks( BinaryPtr bin1, BinaryPtr bin2 )
{
	return compMemBlocks
	(
		bin1->data(), bin1->len(),
		bin2->data(), bin2->len()
	);
}

/**
 *	This tests creating one zip section, adding a child and reading it.
 */
TEST(ZipSection_TestCreate)
{
	BWResource::instance().purgeAll();	// Clear any cached values	
		
	const uint32		duplicateCount	= 10;
	BWRandom			random;	
	std::string			saveAsFilename	= "test_path/test2/test_create.zip";
	std::string			testPath		= "test_path";
	std::string			testTag			= "test_tag.zip";
	std::string			filename		= testPath + "/" + testTag;
	std::string			testString		= "test_string";
	std::string			xmlSectionName	= "testSection";
	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();
	std::vector<BinaryPtr>	data;
	BinaryPtr			origData;
	int					testInt = random(1, INT_MAX);
	bool				ret = false;

	// Delete the sections that will be created again here.
	fileSystem->eraseFileOrDirectory( saveAsFilename );
	fileSystem->eraseFileOrDirectory( filename );

	///////////////////////////////////////////////////////////////////////////
	// Test Create
	{
		DataSectionPtr zipSection = BWResource::openSection( filename, true,
												ZipSection::creator() );
		CHECK(zipSection);
		CHECK(zipSection->sectionName() == testTag);
		CHECK(zipSection->countChildren() == 0); //make sure it wasnt loaded.
	
		//TODO: Add a packed section child

		// Add a bin section child
		DataSectionPtr binChild =
			zipSection->openSection( "binChild", true, BinSection::creator() );
		CHECK(binChild);
		origData = randomBlock(random);
		binChild->setBinary( origData );

		// Add a xml section child
		DataSectionPtr xmlChild =
			zipSection->openSection( "xmlChild", true, XMLSection::creator() );
		CHECK(xmlChild);
		CHECK(xmlChild->countChildren() == 0);

		DataSectionPtr testXML = xmlChild->openSection( xmlSectionName, true, XMLSection::creator() );
		ret = testXML->writeString( "testString", testString );
		CHECK(ret);
		ret = testXML->writeInt( "testInt", testInt );
		CHECK(ret);

		//TODO: Add a dir section child
		//DataSectionPtr dirChild =
		//	zipSection->openSection("dirChild", true, DirSection::creator());
		//CHECK(dirChild);

		// Add a nested zip section child
		DataSectionPtr zipChild =
			zipSection->openSection( "zipChild", true, ZipSection::creator() );
		CHECK(zipChild);

		zipChild = zipChild->openSection( "zipChild2", true, ZipSection::creator() );
		CHECK( zipChild );

		zipChild = zipChild->openSection( "zipChild3", true, ZipSection::creator() );
		CHECK( zipChild );


		// Add some sections with duplicate names
		for (size_t i=0; i<duplicateCount; i++)
		{
			DataSectionPtr nestedBin = zipChild->newSection( "nestedBin", BinSection::creator() );
			CHECK(nestedBin);
			BinaryPtr bin = randomBlock( random );
			data.push_back( bin );
			nestedBin->setBinary( bin );
		}
		CHECK(data.size() == duplicateCount);

		// write out empty binary data
		BinaryPtr empty = new BinaryBlock( "", 0, "test" );
		CHECK(zipSection->writeBinary( "empty", empty ));

		// Test regular save
		ret = zipSection->save();
		CHECK(ret);

		// Test save-as
		ret = zipSection->save(saveAsFilename);
		CHECK(ret);
	}

	///////////////////////////////////////////////////////////////////////////
	// Test Load

	// Load and check the values.
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// Read in the written files and compare the on-disk values with the values
		// in memory.
		DataSectionPtr saveAsSection = BWResource::openSection(saveAsFilename);
		CHECK(saveAsSection);

		DataSectionPtr zipSection = BWResource::openSection(filename);
		CHECK(zipSection);

		// verify the empty binary data we wrote
		BinaryPtr empty = zipSection->readBinary( "empty" );
		CHECK(empty.hasObject() && empty->len() == 0);

		// Load the bin section child
		//DataSectionPtr binChild = zipSection->openSection( "binChild", false, BinSection::creator() );
		DataSectionPtr binChild = zipSection->openSection( "binChild" );
		CHECK(binChild);		
		BinaryPtr binData = binChild->asBinary();
		CHECK(binData);
		CHECK(checkBlocks(binData, origData));

		// Load the xml section child
		DataSectionPtr xmlChild = zipSection->openSection( "xmlChild" );
		CHECK(xmlChild);
	
		DataSectionPtr testXML = xmlChild->openSection( xmlSectionName );
		CHECK(testXML);
		std::string inString = testXML->readString( "testString", "" );
		CHECK(inString == testString);		
		int inVal = testXML->readInt( "testInt", 0 );
		CHECK(inVal == testInt);

		//TODO: Load the dir section child
		//DataSectionPtr dirChild =
		//	zipSection->openSection( "dirChild" );
		//CHECK(dirChild);

		// Load the nested zip section child
		DataSectionPtr zipChild =
		zipSection->openSection( "zipChild" );
		CHECK(zipChild);

		zipChild = zipChild->openSection( "zipChild2/zipChild3" );
		CHECK(zipChild);

		CHECK(zipChild->countChildren());

		// Test the duplicate name sections
		std::vector<DataSectionPtr> sections;
		zipChild->openSections( "nestedBin", sections, BinSection::creator() );
		CHECK(sections.size() == duplicateCount);
		CHECK(data.size() == duplicateCount);

		//TODO: is creation order meant to be guaranteed with sections of the same name?
		for (size_t i=0; i<duplicateCount; i++)
		{
			DataSectionPtr nestedBin = sections[i];
			CHECK(nestedBin);
			BinaryPtr bin = nestedBin->asBinary();
			CHECK(bin);
			CHECK(checkBlocks(bin, data[i]));
		}

		zipChild->deleteSections( "nestedBin" );
		CHECK(zipChild->countChildren() == 0);
	}

	BWResource::instance().purgeAll();	// Clear any cached values
}


bool checkChildExists(DataSectionPtr pSection, const std::string& sectionName)
{
	bool found = false;
	if (pSection)
	{
		for (uint i=0; i<(uint)pSection->countChildren() && !found; i++)
		{
			found = (sectionName == pSection->childSectionName(i));
		}
	}
	return found;
}

TEST(ZipSection_TestDelete)
{
	BWResource::instance().purgeAll();	// Clear any cached values	
		
	const uint32		duplicateCount	= 10;
	BWRandom			random;	
	std::string			emptyName		= "test_deleted.zip";
	std::string			testPath		= "test_path";
	std::string			testTag			= "test_delete.zip";
	std::string			filename		= testPath + "/" + testTag;
	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();
	std::vector<BinaryPtr>	data;
	BinaryPtr			origData;
	bool				ret = false;

	// Delete the sections that will be created again here.
	fileSystem->eraseFileOrDirectory( emptyName );
	fileSystem->eraseFileOrDirectory( filename );
	{
		DataSectionPtr zipSection = BWResource::openSection( filename, true,
												ZipSection::creator() );
		CHECK(zipSection);
		CHECK(zipSection->sectionName() == testTag);
		CHECK(zipSection->countChildren() == 0); //make sure it wasnt loaded.

		// Add a nested zip section child
		DataSectionPtr zipChild =
			zipSection->openSection( "zipChild", true, ZipSection::creator() );
		CHECK(zipChild);

		DataSectionPtr child1 = zipChild->newSection( "child1", BinSection::creator() );
		CHECK(child1);
		BinaryPtr bin = randomBlock( random );
		child1->setBinary( bin );
		data.push_back(bin);

		DataSectionPtr child2 = zipChild->newSection( "child2", BinSection::creator() );
		CHECK(child2);
		bin = randomBlock( random );
		child2->setBinary( bin );
		data.push_back(bin);

		DataSectionPtr child3 = zipChild->newSection( "child3", BinSection::creator() );
		CHECK(child3);
		bin = randomBlock( random );
		child3->setBinary( bin );
		data.push_back(bin);

		// Add some sections with duplicate names
		for (size_t i=0; i<duplicateCount; i++)
		{
			DataSectionPtr nestedBin = zipChild->newSection( "nestedBin", BinSection::creator() );
			CHECK(nestedBin);
			BinaryPtr bin = randomBlock( random );
			nestedBin->setBinary( bin );
		}

		// regular save
		ret = zipSection->save();
		CHECK(ret);
	}

	// Load and check the values.
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// Read in the written files and compare the on-disk values with the values
		// in memory.
		DataSectionPtr zipSection = BWResource::openSection(filename);
		CHECK(zipSection);

		// Load the nested zip section child
		DataSectionPtr zipChild =
		zipSection->openSection( "zipChild" );
		CHECK(zipChild);
		CHECK(zipChild->countChildren() == (duplicateCount+3));

		DataSectionPtr child1 = zipChild->openSection("child1");
		CHECK(child1);
		CHECK( zipChild->deleteSection( child1->sectionName() ) );
		CHECK( zipChild->deleteSection( "child2" ) );
		CHECK( zipChild->deleteSection( "child3" ) );

		zipChild->deleteSections( "nestedBin" );
		CHECK(zipChild->countChildren() == 0);

		// add a single child
		DataSectionPtr pChild = zipChild->newSection( 
								"pChild", BinSection::creator() );
		CHECK(pChild);
		BinaryPtr bin = randomBlock( random );
		pChild->setBinary( bin );
		data.push_back(bin);

		CHECK(zipSection->save(emptyName));
	}

	// Load again to check the saved file is empty
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr zipSection = BWResource::openSection(emptyName);
		CHECK(zipSection);

		DataSectionPtr zipChild =
		zipSection->openSection( "zipChild" );
		CHECK(zipChild);
		CHECK(zipChild->countChildren() == 1);

		DataSectionPtr pChild = zipChild->openSection( "pChild" );
		CHECK(pChild);
		BinaryPtr bin = pChild->asBinary();
		CHECK(bin);
		CHECK(checkBlocks(bin, data[3]));
	}

	// A test very similar to the terrain data saving process currently:
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr cDataDS = BWResource::openSection( "blank.cdata" );
		CHECK(cDataDS);
		//if ( cDataDS )
		{
			DataSectionPtr blockDS = cDataDS->openSection( "terrain2" );
			CHECK(blockDS);
			uint childCount = blockDS->countChildren();
			//if ( blockDS )
			{
				{
					blockDS->deleteSections( "heights" ); //only one
					CHECK(((uint32)blockDS->countChildren()) == (childCount-1));
					CHECK(!checkChildExists(blockDS, "heights"));
					{
						DataSectionPtr pSection = blockDS->newSection(
											"heights", BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(((uint32)blockDS->countChildren()) == childCount);
					CHECK(checkChildExists(blockDS, "heights"));
				}

				{
					blockDS->deleteSections( "normals" );
					CHECK(((uint32)blockDS->countChildren()) == (childCount-1));
					CHECK(!checkChildExists(blockDS, "normals"));

					blockDS->deleteSections( "lodNormals" );
					CHECK(((uint32)blockDS->countChildren()) == (childCount-2));
					CHECK(!checkChildExists(blockDS, "lodNormals"));
					{
						DataSectionPtr pSection = blockDS->newSection(
											"normals", BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(((uint32)blockDS->countChildren()) == (childCount-1));
					CHECK(checkChildExists(blockDS, "normals"));

					{
						DataSectionPtr pSection = blockDS->newSection(
										"lodNormals", BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(((uint32)blockDS->countChildren()) == childCount);
					CHECK(checkChildExists(blockDS, "lodNormals"));
				}

				blockDS->deleteSections( "horizonShadows" );
				CHECK(((uint32)blockDS->countChildren()) == (childCount-1));
				CHECK(!checkChildExists(blockDS, "horizonShadows"));

				{
					DataSectionPtr pSection = blockDS->newSection(
									"horizonShadows", BinSection::creator());
					CHECK(pSection);
					BinaryPtr bin = randomBlock( random );
					pSection->setBinary( bin );
				}
				CHECK(((uint32)blockDS->countChildren()) == childCount);
				CHECK(checkChildExists(blockDS, "horizonShadows"));
				CHECK( cDataDS->save("delete_result.cdata") );
			}
		}
	}

	// A test very similar to the terrain data saving process currently:
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr cDataDS = BWResource::openSection( "testA.cdata",
													true,
													ZipSection::creator() );
		CHECK(cDataDS);
		//if ( cDataDS )
		{
			DataSectionPtr blockDS = cDataDS->newSection( "terrain2",
													ZipSection::creator());
			CHECK(blockDS);
			uint childCount = blockDS->countChildren();
			CHECK(childCount==0);
			//if ( blockDS )
			{
				{
					blockDS->deleteSections( "heights" ); //only one
					CHECK(!checkChildExists(blockDS, "heights"));
					{
						DataSectionPtr pSection = blockDS->newSection("heights",
														BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(checkChildExists(blockDS, "heights"));
				}

				{
					blockDS->deleteSections( "normals" );
					CHECK(!checkChildExists(blockDS, "normals"));

					blockDS->deleteSections( "lodNormals" );
					CHECK(!checkChildExists(blockDS, "lodNormals"));
					{
						DataSectionPtr pSection = blockDS->newSection("normals", BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(checkChildExists(blockDS, "normals"));
					{
						DataSectionPtr pSection = blockDS->newSection("lodNormals", BinSection::creator());
						CHECK(pSection);
						BinaryPtr bin = randomBlock( random );
						pSection->setBinary( bin );
					}
					CHECK(checkChildExists(blockDS, "lodNormals"));
				}

				blockDS->deleteSections( "horizonShadows" );
				CHECK(!checkChildExists(blockDS, "horizonShadows"));
				{
					DataSectionPtr pSection = blockDS->newSection("horizonShadows", BinSection::creator());
					CHECK(pSection);
					BinaryPtr bin = randomBlock( random );
					pSection->setBinary( bin );
				}
				CHECK(checkChildExists(blockDS, "horizonShadows"));				
			}
		}		
		DataSectionPtr pSection = cDataDS->newSection("bin1", BinSection::creator());
		CHECK(pSection);
		BinaryPtr bin = randomBlock( random );
		pSection->setBinary( bin );

		CHECK( cDataDS->save("delete_and_save_result.cdata") );
	}


	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr cDataDS = BWResource::openSection( "delete_and_save_result.cdata" );
		CHECK(cDataDS);
		//if ( cDataDS )
		{
			DataSectionPtr blockDS = BWResource::openSection( "delete_and_save_result.cdata/terrain2" );
			CHECK(blockDS);
			//if ( blockDS )
			{
				CHECK(checkChildExists(blockDS, "heights"));
				CHECK(checkChildExists(blockDS, "normals"));
				CHECK(checkChildExists(blockDS, "lodNormals"));
				CHECK(checkChildExists(blockDS, "horizonShadows"));				
			}
		}
		CHECK(checkChildExists(cDataDS, "bin1"));
	}

	BWResource::instance().purgeAll();	// Clear any cached values
}


TEST(ZipSection_TestConversions)
{
	BWRandom			random;
	std::string			saveAsFilename	= "test_conversion.zip";
	std::string			saveAsFilename2	= "test_conversion2.zip";
	bool				ret = false;
	BinaryPtr			origData;

	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();

	// Delete the sections that will be created again here.
	fileSystem->eraseFileOrDirectory( saveAsFilename );
	fileSystem->eraseFileOrDirectory( saveAsFilename2 );

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// Load, modify and save a zip file with directories
		DataSectionPtr binSection = BWResource::openSection( "test_bin.bin",
										true, BinSection::creator() );
		CHECK(binSection);

		DataSectionPtr pTest = binSection->newSection( "test",
									BinSection::creator() );
		CHECK(pTest);
		
		pTest->deleteSections( "binChild" );
		DataSectionPtr binChild = pTest->newSection( "binChild" );
		CHECK(binChild);	
		origData = randomBlock(random);
		binChild->setBinary( origData );	

		DataSectionPtr zipSection = binSection->convertToZip();
		CHECK(zipSection);
		CHECK(zipSection.getObject() != binSection.getObject());

		ret = zipSection->save(saveAsFilename);
		CHECK(ret);
	
		ret = binChild->save(saveAsFilename2);
		CHECK(ret);

		DataSectionPtr pChild = zipSection->openSection( "test" );
		CHECK(pChild);

		// if this is already a zip, it will return itself.
		DataSectionPtr pTestChild = pChild->convertToZip();
		CHECK(pTestChild);
		CHECK(pTestChild.getObject() == pChild.getObject());
	}

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr zipSection = BWResource::openSection(saveAsFilename);
		CHECK(zipSection);
		DataSectionPtr binSection = BWResource::openSection(saveAsFilename2);
		CHECK(binSection);
		DataSectionPtr pChild = zipSection->openSection( "test" );
		CHECK(pChild);
		DataSectionPtr binChild = pChild->openSection( "binChild" );
		CHECK(binChild);
	}
	
	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// load a binary section as if it were a zip section
		DataSectionPtr binSection = BWResource::openSection( "test_binary",
										true, ZipSection::creator() );
		CHECK(binSection);
	}

	BWResource::instance().purgeAll();	// Clear any cached values
}

TEST(ZipSection_TestNestedDirectory)
{
	BWRandom			random;
	std::string			saveAsFilename	= "test_path/test2/test_dir_result.zip";
	std::string			testString		= "test_string";
	std::string			xmlSectionName	= "testSection";
	int					testInt = random(1, INT_MAX);
	bool				ret = false;
	const uint32		duplicateCount	= 10;
	BinaryPtr			origData;

	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();

	// Delete the sections that will be created again here.
	fileSystem->eraseFileOrDirectory( saveAsFilename );

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		// Load, modify and save a zip file with directories
		DataSectionPtr zipSection = BWResource::openSection( "test_path/test_dir.zip" );
		CHECK(zipSection);

		// Load the nested zip section child
		DataSectionPtr zipChild =
		zipSection->openSection( "zipChild" );
		CHECK(zipChild);

		// Test the duplicate name sections
		std::vector<DataSectionPtr> sections;
		zipChild->openSections( "nestedBin", sections, BinSection::creator() );
		CHECK(sections.size() == duplicateCount);

		for (size_t i=0; i<duplicateCount; i++)
		{
			DataSectionPtr nestedBin = sections[i];
			CHECK(nestedBin);
			BinaryPtr bin = nestedBin->asBinary();
			CHECK(bin);
		}

		DataSectionPtr nestedXMLChild =
			zipSection->openSection( "zipChild/xmlChild", true, XMLSection::creator() );
		CHECK(nestedXMLChild);
		CHECK(nestedXMLChild->countChildren() == 0);

		DataSectionPtr testXML2 = nestedXMLChild->openSection( xmlSectionName, true, XMLSection::creator() );
		CHECK(testXML2);
		ret = testXML2->writeString( "testString", testString );
		CHECK(ret);
		ret = testXML2->writeInt( "testInt", testInt );
		CHECK(ret);

		ret = zipSection->save(saveAsFilename);
		CHECK(ret);
	}

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr zipSection = BWResource::openSection(saveAsFilename);
		CHECK(zipSection);

		DataSectionPtr nestedXMLChild = zipSection->openSection( "zipChild/xmlChild" );
		CHECK(nestedXMLChild);

		DataSectionPtr testXML = nestedXMLChild->openSection( xmlSectionName );
		CHECK(testXML);
		std::string test = testXML->readString( "testString", "" );
		CHECK(test == testString);
		int test2 = testXML->readInt( "testInt", 0 );
		CHECK( test2 == testInt );
	}

	BWResource::instance().purgeAll();	// Clear any cached values
}

/**
 *	This tests a particular case where a zip was loading the same child 
 *	section twice, causing a crash when you try to delete the section.
 */
TEST( ZipSection_DupChild )
{
	std::string filename = "dup_child_test.zip";
	std::string saveFilename = "dup_child_test_save.zip";

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr zipSection = BWResource::openSection(filename);
		CHECK(zipSection);
		CHECK(zipSection->findChild("lighting"));
		
		zipSection->delChild("lighting");

		CHECK(!zipSection->findChild("lighting"));
		CHECK(zipSection->save(saveFilename));
	}

	BWResource::instance().purgeAll();	// Clear any cached values
	{
		DataSectionPtr zipSection = BWResource::openSection(saveFilename);
		CHECK(zipSection);
		CHECK(!zipSection->findChild("lighting"));		
	}
}

// test_zip_section.cpp
