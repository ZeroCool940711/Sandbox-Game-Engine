


root = None # This is the root data section of the resource tree. Type: DataSection  

class DataSection:
	"""A DataSection controls the reading and writing of xml data sections.
	New DataSections can be created via ResMgr.DataSection(). In this case
	section cannot be saved because it has no file system association. To open
	an existing DataSection, use ResMgr.openSection(). """
	
	asBinary  = None		# String
	asBlob    = None 		#String  
	asBool    = None  		#Integer  
	asFloat   = None  		#Float  
	asInt     = None  		#Integer  
	asInt64   = None  		#Integer  
	asMatrix  = None  		#Matrix  
	asString  = None  		#String  
	asVector2 = None  		#Vector2  
	asVector3 = None  		#Vector3  
	asVector4 = None 		#Vector4  
	asWideString = None  		#Wide String  
	name = None  			#Read-Only String  
	name = None  			#Read-Only String  
	
	"""asBinary: 
	This attribute is the underlying binary data of this DataSection, encapsulated in
	a string. On reading, it includes all the child sections. On writing, it may invalidate
	previously accessed child sections. Use with care. It is better to use the 'copy' method
	to copy DataSections around than to use this attribute. 
	Type: String  

	asBlob: 
	This attribute is the value of this DataSection, interpreted as a BASE64 string. When
	reading it, it doesn't include any of the child sections. Also on reading, the leading
	and trailing white space is trimmed off the string. On writing, it doesn't effect any of
	the child nodes. 
	Type: String  

	asBool: 
	This attribute is the value of this DataSection, interpreted as a boolean. 

	On reading, if the text contains "true" in any capitalisation, it returns 1, otherwise
	it returns 0. 

	On writing, if it is given a 0, it writes "false" otherwise it writes "true". 
	Type: Integer  

	asFloat:
	This attribute is the value of this DataSection, interpreted as an float. On reading,
	it reads the string from the top level, and tries to parse a float off the front of it.
	If this is successful, the float is returned, otherwise zero. 

	On writing, it doesn't effect any of the child nodes, but replaces the top level
	string with the string version of the float. 
	Type: Float  

	asInt: 
	This attribute is the value of this DataSection, interpreted as an integer. On reading,
	it reads the string from the top level, and tries to parse an integer off the front of
	it. If this is successful, the integer is returned, otherwise zero. 

	On writing, it doesn't effect any of the child nodes, but replaces the top level string
	with the string version of the integer. 
	Type: Integer  

	asInt64: 
	This attribute is the value of this DataSection, interpreted as a 64 bit integer. On
	reading, it reads the string from the top level and tries to parse an integer off the
	front of it. If this is successful, the integer is returned, otherwise zero. 

	On writing, it doesn't effect any of the child nodes, but replaces the top level string
	with the string version of the integer. 
	Type: Integer  

	asMatrix: 
	This attribute is the value of this DataSection, interpreted as a Matrix. 

	On reading, it checks for four sub-sections, called , , and . If each of these exist and
	contain three floats separated by whitespace, then they are interpreted as the four rows
	of a Matrix, which is created and returned. Otherwise, the Matrix of all zeros is returned. 

	On Writing, it accepts a Matrix, and writes it out to the four sub-sections , , and , each
	of which will contain one row of the Matrix - three floats separated by white space. This
	will not effect any top level text stored in this DataSection. 
	Type: Matrix  

	asString: 
	This attribute is the value of this DataSection, interpreted as a string. When reading
	it, it doesn't include any of the child sections. Also on reading, the leading and trailing
	white space is trimmed off the string. On writing, it doesn't effect any of the child nodes. 
	Type: String  

	asVector2: 
	This attribute is the value of this DataSection, interpreted as a Vector2. On reading, it
	reads the string from the top level, and tries to parse it as two floats, separated by
	white space. If this is successful, the Vector2 of these floats is returned, otherwise 
	the Vector2 (0,0). 

	On writing, it doesn't effect any of the child nodes, but replaces the top level string 
	with the string version of the two components of the Vector2 as floats, separated by white space. 
	Type: Vector2  

	asVector3: 
	This attribute is the value of this DataSection, interpreted as a Vector3. On reading,
	it reads the string from the top level, and tries to parse it as three floats, separated
	by white space. If this is successful, the Vector3 of these floats is returned, otherwise
	the Vector3 (0,0,0). 
	
	On writing, it doesn't effect any of the child nodes, but replaces the top level string
	with the string version of the three components of the Vector3 as floats, separated by
	white space. 
	Type: Vector3  
	
	
	asVector4: 
	This attribute is the value of this DataSection, interpreted as a Vector4. On reading, it
	reads the string from the top level, and tries to parse it as four floats, separated by white
	space. If this is successful, the Vector4 of these floats is returned, otherwise the Vector4 (0,0,0,0). 
	
	On writing, it doesn't effect any of the child nodes, but replaces the top level string with
	the string version of the four components of the Vector4 as floats, separated by white space. 
	Type: Vector4  
	
	
	asWideString: 
	This attribute is the value of this DataSection, interpreted as a wide string. When reading
	it, it doesn't include any of the child sections. Also on reading, the leading and trailing
	white space is trimmed off the string. On writing, it doesn't effect any of the child nodes. 
	Type: Wide String  
	
	
	name: 
	This attribute is the name of the data section. It is read-only, and consists of the tag
	surrounding this particular section 
	Type: Read-Only String  
	"""
	
	def child( self, index ): 
		"""This method returns the child at the given index. 
		Parameters: index  the index to look up the child subsection at.  
		
		
		Returns: the child at the specified index."""
		return
	
	def childName( self, index ):
		"""This method returns the name of the child at the given index. 
		Parameters: index  the index to look up the child subsection at.  
		
		
		Returns: the child at the specified index."""
		return
	
	def cleanName( self ):
		"""This method cleans xml section names by replacing spaces with
		double periods and puts "id." in front of xml section names that begin with numbers. 
		Returns: A boolean indicating that a change took place in the name."""
		return
	
	def copy( self, source ):
		"""This function makes this DataSection into a copy of the specified DataSection. 
		Parameters: source  the section to copy this one from."""
		return
	
	def copyToZip( self ):
		"""This function makes takes this DataSection and copies it into a new DataSection
		as well as converting it to a zip if possible. 
		Returns: A 2-tuple with the new data section and a boolean to indicate if the
		conversion was successful."""
		return
	
	def createSection( self, sectionPath ):
		"""This method creates the section specified by the input path, as a subsection
		of the parent. It will always create a new subsection, even if a section of the
		same name already exists. The path can contain slashes, which will allow for the
		creation of subsection within subsection. The path will be evaluated through the
		first section that has that name, and, if the whole path already exists, only the
		innermost section will be duplicated. 

		For example, if ds represents the following piece of xml: 


		<red>
			<green>
				<blue></blue>
			</green>
		</red>
		<red>
			<green>
				<blue></blue>
			</green>
		</red>



		and then the following code is executed: 


		ds.createSection( "red/green/blue" )



		then the following will be the resultant xml represented by ds: 


		<red>
			<green>
				<blue></blue>
				<blue></blue>
			</green>
		</red>
		<red>
			<green>
				<blue></blue>
			</green>
		</red>

		Parameters: sectionPath  the path to the section to create.  


		Returns: the DataSection that was created."""
		return
	
	def createSectionFromString( self, string ):
		"""This method creates the section represented by the input string, as a subsection of the parent. 

		For example, if ds represents the following piece of xml: 


		<red>
			<green>
				<blue></blue>
			</green>
		</red>



		and then the following code is executed: 


		string = "<red><green><blue></blue></green></red>"
		ds.createSectionFromString( string )


		then the following will be the resultant xml represented by ds: 


		<red>
			<green>
				<blue></blue>
			</green>
		</red>
		<red>
			<green>
				<blue></blue>
			</green>
		</red>

		Parameters: string  the xml string to create the section from.  


		Returns: the DataSection that was created."""
		
		return
	
	def deleteSection( self, section ):
		"""This method deletes a section referenced either by a PyDataSection
		or an input string path. in the case of input string path,if more than
		one section is addressed by the path, then only the first one is deleted.
		It returns 1 if the section was successfully deleted, 0 otherwise. 
		Parameters: section  can be either a PyDataSection or a string path of 
		the section to delete  


		Returns: 1 if it was deleted, 0 otherwise."""
		return
	
	def has_key( self, key ):
		"""This method checks if one of the subsections within this data
		section has the specified name, and returns 1 if it does, 0 otherwise. 
		Parameters: key  A string containing the name of the section to check for.  


		Returns: An integer: 1 if the section exists, 0 otherwise."""
		return
	
	def isNameClean( self ):
		"""This method returns whether the section name is valid. XML can be
		a little more strict about allowed names. 
		Returns: A boolean indicating whether the name is clean."""
		return
	
	def items( self ):
		"""Returns: The list of (name, DataSection) tuples of this section's children."""
		return
	
	def keys( self ):
		"""This method returns a list of all the subsection names, in the order 
		in which they appear in the xml code. 
		Returns: The list of the names (strings)."""
		return
	
	def readBlob( self, sectionName, defaultVal ):
		"""This function returns the contents of the section with the name specified,
		interpreted as a Blob string. It performs a trim (drop off leading and trailing
		white space) on the text immediately following the opening tag, and preceding the
		first open tag for a subsection. 

		If there is more than one section with the specified name, then only the first
		one is used. 
		Parameters: sectionName  the name of the section to get the string from.  
		defaultVal  the default value should an error occur.  


		Returns: the string contained in the specified section, or the default
		value (if specified), an empty string otherwise."""
		return
	
	def readBool( self, sectionName, defaultVal ):
		"""This function returns the contents of the section with the name specified,
		parsed as a boolean. 

		If it contains the string "true", with any capitalisation, then it returns 1,
		otherwise, it returns 0. 

		If there is more than one section with the specified name, then only the first
		one is used. 
		Parameters: sectionName  the name of the section to get the boolean from.  
		defaultVal  the default value should an error occur.  


		Returns: the boolean (integer) contained in the specified section, or the
		default value (if specified), false otherwise."""
		return
	
	def readFloat( self, sectionName, defaultVal ):
		"""This function returns the contents of the section with the name specified
		parsed as a floating point number. 

		It performs a trim (drop off leading and trailing white space) on the text
		immediately following the opening tag, and preceding the first open tag for
		a subsection. It tries to parse a float at the front of the resulting string,
		returning the integer if the parse succeeds, 0 otherwise. 

		If the sectionName specified doesn't match any sections, then 0 is returned. 

		If there is more than one section with the specified name, then only the
		first one is used. 
		Parameters: sectionName  the name of the section to get the float from.  
		defaultVal  the default value should an error occur.  


		Returns: the float contained in the specified section, or the default value 
		(if specified), 0 otherwise."""
		return
	
	def readFloats( self, sectionName ):
		"""This function reads the contents of any subsections of the current
		section with the specified name, and returns the results as tuple of floats.
		It tries to parse the contents of each subsection as a float, using that
		float if the parse is sucessful, otherwise 0. 
		Parameters: sectionName  the name of the subsections from which floats
		will be parsed.  


		Returns: a tuple of the floats from the named subsections."""
		return
	
	def readInt( self, sectionName, defaultVal ):
		"""This function returns the contents of the section with the name
		specified parsed as an integer. 

		It performs a trim (drop off leading and trailing white space) on the text
		immediately following the opening tag, and preceding the first open tag for
		a subsection. It tries to parse an integer at the front of the resulting string,
		returning the integer if the parse succeeds, 0 otherwise. 

		If the sectionName specified doesn't match any sections, then 0 is returned. 

		If there is more than one section with the specified name, then only the
		first one is used. 
		Parameters: sectionName  the name of the section to get the int from.  
		defaultVal  the default value should an error occur.  


		Returns: the int contained in the specified section, or the default value
		(if specified), 0 otherwise."""
		return
	
	def readInt64( self, sectionName, defaultVal ):
		"""This function returns the contents of the section with the name
		specified parsed as an integer. 

		It performs a trim (drop off leading and trailing white space) on the
		text immediately following the opening tag, and preceding the first open 
		tag for a subsection. It tries to parse an integer at the front of the
		resulting string, returning the integer if the parse succeeds, 0 otherwise. 

		If the sectionName specified doesn't match any sections, then 0 is returned. 

		If there is more than one section with the specified name, then only the 
		first one is used. 
		Parameters: sectionName  the name of the section to get the int from.  
		defaultVal  the default value should an error occur.  


		Returns: the int contained in the specified section, or the default
		value (if specified), 0 otherwise."""
		return
	
	def readInts( self, sectionName ):
		"""This function reads the contents of any subsections of the current
		section with the specified name, and returns the results as tuple of integers. 
		It tries to parse the contents of each subsection as an integer, using 
		that integer if the parse is sucessful, otherwise 0. 
		Parameters: sectionName  the name of the subsections from which integers
		will be parsed.  


		Returns: a tuple of the integers from the named subsections."""
		return
	
	def readMatrix( self, sectionName, defaultVal ):
		"""This function returns the contents of the section with the name
		specified parsed as a Matrix. 

		It expects there there to be four subsections, called <row0>, <row1>,
		<row2> and <row3>. Each of these should have three floats separated by
		white space. If these conditions are met, a Matrix will be read in from the
		four rows otherwise, a Matrix with all zeros will be created. 

		If there is more than one section with the specified name, then only the first 
		one is used. 
		Parameters: sectionName  the name of the section to get the Matrix from.  
		defaultVal  the default value should an error occur.  


		Returns: the Matrix contained in the specified section, or the default value
		(if specified), a Null Matrix otherwise."""
		return
	
	def readString( self, sectionName, defaultVal ):
		"""This function returns the contents of the section with the name specified,
		interpreted as a string. It performs a trim (drop off leading and trailing white
		space) on the text immediately following the opening tag, and preceding the first
		open tag for a subsection. 

		If there is more than one section with the specified name, then only the first 
		one is used. 
		Parameters: sectionName  the name of the section to get the string from.  
		defaultVal  the default value should an error occur.  

		
		Returns: the string contained in the specified section, or the default value
		(if specified), an empty string otherwise."""
		return
	
	def readStrings( self, sectionName ):
		"""This function reads the contents of any subsections of the current section
		with the specified name, and returns the results as tuple of strings. 
		Parameters: sectionName  the name of the subsections from which strings will be extracted.  


		Returns: a tuple of the strings from the named subsections."""
		return
	
	def readVector2( self, sectionName, defaultVal ):
		"""This function returns the contents of the section with the name specified parsed
		as a Vector2. 

		It expects two floating point numbers, separated by white space, at the beginning of
		the text for the named section. Any trailing text is ignored. If it gets this, it
		returns the two floats as a Vector2, otherwise it returns (0, 0). 

		If there is more than one section with the specified name, then only the first one is used. 
		Parameters: sectionName  the name of the section to get the Vector2 from.  
		defaultVal  the default value should an error occur.  


		Returns: the Vector2 contained in the specified section, or the default value
		(if specified), (0, 0) otherwise."""
		return
	
	def readVector2s( self, sectionName ):
		"""This function reads the contents of any subsections of the current section with the
		specified name, and returns the results as tuple of Vector2s. It tries to parse the 
		contents of each subsection as two floats, separated by whitespace. If the parse is 
		successful, then these two floats are interepreted as a Vector2 and added to the
		tupple, otherwise the Vector2 (0,0) is used instead. 
		Parameters: sectionName  the name of the subsections from which Vector2s will be parsed.  


		Returns: a tuple of the Vector2s from the named subsections."""
		return
	
	def readVector3( self, sectionName, defaultVal ):
		"""This function returns the contents of the section with the name specified
		parsed as a Vector3. 

		It expects three floating point numbers, separated by white space, at the beginning
		of the text for the named section. Any trailing text is ignored. If it gets this, 
		it returns the three floats as a Vector3, otherwise it returns (0, 0, 0). 

		If there is more than one section with the specified name, then only the first one is used. 
		Parameters: sectionName  the name of the section to get the Vector3 from.  
		defaultVal  the default value should an error occur.  


		Returns: the Vector3 contained in the specified section, or the default value
		(if specified), (0, 0, 0) otherwise."""
		return
	
	def readVector3s( self, sectionName ):
		"""This function reads the contents of any subsections of the current section
		with the specified name, and returns the results as tuple of Vector3s. It tries
		to parse the contents of each subsection as three floats, separated by whitespace.
		If the parse is successful, then these three floats are interepreted as a Vector3
		and added to the tupple, otherwise the Vector3 (0,0,0) is used instead. 
		Parameters: sectionName  the name of the subsections from which Vector3s will be parsed.  


		Returns: a tuple of the Vector3s from the named subsections."""
		return
	
	def readVector4( self, sectionName, defaultVal ):
		"""This function returns the contents of the section with the name specified
		parsed as a Vector4. 

		It expects four floating point numbers, separated by white space, at the
		beginning of the text for the named section. Any trailing text is ignored. 
		If it gets this, it returns the four floats as a Vector4, otherwise it returns (0, 0, 0, 0). 

		If there is more than one section with the specified name, then only the first one is used. 
		Parameters: sectionName  the name of the section to get the Vector4 from.  
		defaultVal  the default value should an error occur.  


		Returns: the Vector4 contained in the specified section, or the default value
		(if specified), (0, 0, 0, 0) otherwise."""
		return
	
	def readVector4s( self, sectionName ):
		"""This function reads the contents of any subsections of the current section with
		the specified name, and returns the results as tuple of Vector4s. It tries to parse
		the contents of each subsection as four floats, separated by whitespace. If the parse
		is successful, then these four floats are interepreted as a Vector4 and added to the
		tupple, otherwise the Vector4 (0,0,0,0) is used instead. 
		Parameters: sectionName  the name of the subsections from which Vector4s will be parsed.  
		
		
		Returns: a tuple of the Vector4s from the named subsections."""
		return
		
	def readWideString( self, sectionName, defaultVal ): 
		"""This function returns the contents of the section with the name specified,
		assuming that the contents are a wide string. It performs a trim (drop off leading
		and trailing white space) on the text immediately following the opening tag, and preceding
		the first open tag for a subsection. 
		
		Most wide strings in text xml files are encoded as base64. However, you can create
		readable wide strings in xml by hand, simply by prefixing a standard ascii string
		with an exclamation mark. 
		
		If there is more than one section with the specified name, then only the first one is used. 
		Parameters: sectionName  the name of the section to get the wide string from.  
		defaultVal  the default value should an error occur.  
		
		
		Returns: the string contained in the specified section, or the default value
		(if specified), an empty string otherwise.  """
		return
		
		
	def readWideStrings( self, sectionName ): 
		"""This function reads the contents of any subsections of the current 
		section with the specified name, and returns the results as tuple of wide
		strings. It assumes that the contents of those subsections were in wide string format. 
		
		Most wide strings in text xml files are encoded as base64. However, you can 
		create readable wide strings in xml by hand, simply by prefixing a standard
		ascii string with an exclamation mark. 
		Parameters: sectionName  the name of the subsections from which wide strings 
		will be extracted.  
		
		
		Returns: a tuple of the wide strings from the named subsections."""
		return
		
	def save( self ): 
		"""This method saves the data section back to its underlying file. If called
		on a DataSection which does not correspond to a file, then it causes an IO error. """
		return
	
	def values( self ): 
		"""This method returns a list of all child sections, in the order in which they
		appear in the XML. 
		Returns: a list of DataSections."""
		return
		
	def write( self, sectionName, value ): 
		"""This function writes a subsection with the specified name. It creates the
		section if it does not already exist. If one or more sections of that name
		already exist, then it modifies the first one. It writes whatever value it
		was given as its second argument as the contents for the named section. 
		Parameters: sectionName  the name of the subsection to write to.  
		value  the value to write to the named section.  
		
		
		Returns: the DataSection that was written to."""
		return
	
	def writeBlob( self, sectionName, text ): 
		"""This function writes a subsection with the specified name, writing the
		specified BLOB string. If one or more subsections with that name already exist,
		then the value of the first one is replaced with the specified string. This will
		not effect any subsections of the named subsection, only its actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		text  the text to replace the subsection contents with.  
		
		
		Returns: the DataSection that was written to.  """
		return
	
	
	def writeBool( self, sectionName, val ): 
		"""This function writes a subsection with the specified name, writing
		the specified integer as a string. If the integer is zero, 'false' is written,
		otherwise 'true'. If one or more subsections with that name already exist, 
		then the value of the first one is replaced with the specified boolean. This
		will not effect any subsections of the named subsection, only its actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		val  the integer to replace the subsection contents with.  
	
	
		Returns: the DataSection that was written to.""" 
		return
	
	
	def writeFloat( self, sectionName, val ): 
		"""This function writes a subsection with the specified name, writing the
		specified float as a string. If one or more subsections with that name
		already exist, then the value of the first one is replaced with the
		specified float. This will not effect any subsections of the named 
		subsection, only its actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		val  the float to replace the subsection contents with.  
	
	
		Returns: the DataSection that was written to.  """
		return
	
	
	def writeFloats( self, sectionName, values ): 
		"""This function creates a subsection of the specified name for each
		element of the supplied sequence, writing the corresponding float from
		the sequence as the contents of that subsection. 
	
		For example, the following statement: 
	
	
		ds.writeInts( "sect", ( 123.4, 456.7 ) )
	
	
	
		would result in the generation of the following xml 
	
	
		<sect> 123.4 </sect>
		<sect> 456.7 </sect>
	
	
	
		This function has no effect on existing subsections of the same name. 
		A new subsection is always created for each element in the sequence. 
		Parameters: sectionName  the name of the subsection to create.  
		values  the sequence of floats to use as values for the new subsections."""
		return
	
	def writeInt( self, sectionName, val ): 
		"""This function writes a subsection with the specified name, writing 
		the specified integer as a string. If one or more subsections with that 
		name already exist, then the value of the first one is replaced with the
		specified integer. This will not effect any subsections of the named 
		subsection, only its actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		val  the integer to replace the subsection contents with.  
	
	
		Returns: the DataSection that was written to."""
		return
	
	def writeInt64( self, sectionName, val ): 
		"""This function writes a subsection with the specified name, writing the
		specified 64-bit integer. If one or more subsections with that name alreadycl
		exist, then the value of the first one is replaced with the specified value. 
		This will not affect any subsections of the named * subsection, only its actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		val  the integer to replace the subsection contents with.  
	
	
		Returns: the DataSection that was written to."""
		return
	
	def writeInts( self, sectionName, values ): 
		"""This function creates a subsection of the specified name for each element
		of the supplied sequence, writing the corresponding integer from the sequence
		as the contents of that subsection. 
	
		For example, the following statement: 
	
	
		ds.writeInts( "sect", ( 123, 456 ) )
	
	
	
		would result in the generation of the following xml 
	
	
		<sect> 123 </sect>
		<sect> 456 </sect>
	
	
	
		This function has no effect on existing subsections of the same name. A 
		new subsection is always created for each element in the sequence. 
		Parameters: sectionName  the name of the subsection to create.  
		values  the sequence of integers to use as values for the new subsections."""
		return
	
	def writeMatrix( self, sectionName, val ): 
		"""This function writes a subsection with the specified name, writing the
		specified Matrix. If one or more subsections with that name already exist,
		then the value of the first one is replaced with the specified value. This
		will not effect any subsections of the named subsection, only its actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		val  the Matrix to replace the subsection contents with.  
	
	
		Returns: the DataSection that was written to."""
		return
	
	def writeString( self, sectionName, text ): 
		"""This function writes a subsection with the specified name, writing the specified
		string. If one or more subsections with that name already exist, then the value of
		the first one is replaced with the specified string. This will not effect any subsections
		of the named subsection, only its actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		text  the text to replace the subsection contents with.  
	
	
		Returns: the DataSection that was written to."""
		return
	
	def writeStrings( self, sectionName, values ): 
		"""This function creates a subsection of the specified name for each element of the
		supplied list, writing the corresponding string from the list as the contents of that subsection. 
	
		For example, the following statement: 
	
	
		ds.writeStrings( "sect", ( "str1", "str2" ) )
	
	
	
		would result in the generation of the following xml: 
	
	
		<sect> str1 </sect>
		<sect> str2 </sect>
	
	
	
		This function has no effect on existing subsections of the same name. A
		new subsection is always created for each list element. 
		Parameters: sectionName  the name of the subsection to create.  
		values  the list of strings to use as values for the new subsections."""
		return
	
	def writeVector2( self, sectionName, val ): 
		"""This function writes a subsection with the specified name, writing the specified
		Vector2 as a string, consisting of two floats, separated by white space. If one or
		more subsections with that name already exist, then the value of the first one is
		replaced with the specified Vector2. This will not effect any subsections of the
		named subsection, only its actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		val  the Vector2 to replace the subsection contents with.  
	
	
		Returns: the DataSection that was written to."""
		return
	
	def writeVector2s( self, sectionName, values ): 
		"""This function creates a subsection of the specified name for each element
		of the supplied sequence, writing the corresponding Vector2 from the sequence
		as the contents of that subsection. 
	
		For example, the following statement: 
	
	
		ds.writeVector2s( "sect", ( (1.1, 2.2) , (3.3, 4.4) ) )
	
	
	
		would result in the generation of the following xml 
	
	
		<sect> 1.100000 2.200000 </sect>
		<sect> 3.300000 4.400000 </sect>
	
	
	
		This function has no effect on existing subsections of the same name. A new 
		subsection is always created for each element in the sequence. 
		Parameters: sectionName  the name of the subsection to create.  
		values  the sequence of Vector2s to use as values for the new subsections."""
		return
	
	def writeVector3( self, sectionName, val ): 
		"""This function writes a subsection with the specified name, writing
		the specified Vector3 as a string, consisting of three floats, separated
		by white space. If one or more subsections with that name already exist,
		then the value of the first one is replaced with the specified Vector3.
		This will not effect any subsections of the named subsection, only its
		actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		val  the Vector3 to replace the subsection contents with.  
	
	
		Returns: the DataSection that was written to."""
		return
	
	def writeVector3s( self, sectionName, values ): 
		"""This function creates a subsection of the specified name for each element
		of the supplied sequence, writing the corresponding Vector3 from the sequence
		as the contents of that subsection. 
	
		For example, the following statement: 
	
		
		ds.writeVector3s( "sect", ( (1.1, 2.2, 3.3) , (3.3, 4.4, 5.5) ) )
	
	
	
		would result in the generation of the following xml 
	
	
		<sect> 1.100000 2.200000 3.300000 </sect>
		<sect> 3.300000 4.400000 5.500000 </sect>
	
	
	
		This function has no effect on existing subsections of the same name. A new
		subsection is always created for each element in the sequence. 
		Parameters: sectionName  the name of the subsection to create.  
		values  the sequence of Vector3s to use as values for the new subsections."""
		return
	
	def writeVector4( self, sectionName, val ): 
		"""This function writes a subsection with the specified name, writing the specified
		Vector4 as a string, consisting of four floats, separated by white space. If one or
		more subsections with that name already exist, then the value of the first one is
		replaced with the specified Vector4. This will not effect any subsections of the
		named subsection, only its actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		val  the Vector4 to replace the subsection contents with.  
	
		
		Returns: the DataSection that was written to."""
		return
	
	def writeVector4s( self, sectionName, values ): 
		"""This function creates a subsection of the specified name for each element
		of the supplied sequence, writing the corresponding Vector4 from the sequence
		as the contents of that subsection. 
		
		For example, the following statement: 
		
		
		ds.writeVector3s( "sect", ( (1.1, 2.2, 3.3, 4.4) , (3.3, 4.4, 5.5, 6.6) ) )
	
		
		
		would result in the generation of the following xml 
	
	
		<sect> 1.100000 2.200000 3.300000 4.400000 </sect>
		<sect> 3.300000 4.400000 5.500000 6.600000 </sect>
	
	
	
		This function has no effect on existing subsections of the same name. A new 
		subsection is always created for each element in the sequence. 
		Parameters: sectionName  the name of the subsection to create.  
		values  the sequence of Vector4s to use as values for the new subsections.  """
		return
	
	def writeWideString( self, sectionName, text ): 
		"""This function writes a subsection with the specified name, writing the specified
		wide string. If one or more subsections with that name already exist, then the value
		of the first one is replaced with the specified wide string. This will not effect any
		subsections of the named subsection, only its actual contents. 
		Parameters: sectionName  the name of the subsection to create or overwrite.  
		text  the text to replace the subsection contents with.  
	
		
		Returns: the DataSection that was written to."""
		return
		
	def writeWideStrings( self, sectionName, values ): 
		"""This function creates a subsection of the specified name for each element of the
		supplied list, writing the corresponding wide string from the list as the contents of that subsection. 
	
		This function has no effect on existing subsections of the same name. A new subsection
		is always created for each list element. 
		Parameters: sectionName  the name of the subsection to create.  
		values the list of wide strings to use as values for the new subsections."""
		return

	
		
	

def DataSection( sectionName ):
	"""This module function constructs a blank data section. It can be optionally supplied with a new name. 
	This DataSection will not be savable using the save function, as it has no file system association. 
	Parameters: sectionName  Optional name for the new section  
	
	
	Returns: the section that was created."""
	return

def isDir( pathname ):
	"""This function returns true if the specified path name is a directory, false otherwise. 
	
	Parameters: pathname  The path name to check.  
	
	Returns: True (1) if it is a directory, false (0) otherwise."""
	return

def isFile( pathname ):
	"""This function returns true if the specified path name is a file, false otherwise. 
	Parameters: pathname  The path name to check.  
	
	
	Returns: True (1) if it is a file, false (0) otherwise."""
	return

def localise( string, params ):
	"""This function localises a string and expand its params 
	Parameters: string  The string to localise  
	params  The params used to expand  
	
	
	Returns: localised and expanded string."""
	return

def openSection( resourceID, newSection ):
	"""This function opens the specified resource as a DataSection.
	If the resource is not found, then it returns None. A new section
	can optionally be created by specifying true in the optional second argument. 

	Resources live in a res tree and include directories, xml files,
	xml nodes, normal data files, binary section data file nodes, etc. 
	Parameters: resourceID  the id of the resource to open.  
	newSection  Boolean value indicating whether to create this as a new section,
	default is False.  


	Returns: the DataSection that was loaded, or None if the id was not found."""
	return

def purge( resourceID, recurse ):
	"""This function purges the previously loaded section with the specified path
	from the cache and census. Optionally, all child sections can also be purged
	(only useful if the resource is a DirSection), by specifying true in the optional
	second argument. 
	Parameters: resourceID  the id of the resource to purge.  
	recurse  Boolean value indicating whether to recursively purge any subsections.
	default is False."""
	return

def resolveToAbsolutePath( ):
	"""This function resolves a path relative to the res tree to an absolute path.
	If the file does not exist in any of the resource paths, then it will be resolved
	against the first resource path. 
	Returns: The absolute path."""
	return

def save( resourceID ):
	"""This function saves the previously loaded section with the specified path.
	If no section with that id is still in memory, then an IO error occurs, otherwise,
	the section is saved. 
	Parameters: resourceID  the filepath of the DataSection to save.  


	Returns: True if successful, False otherwise."""
	return