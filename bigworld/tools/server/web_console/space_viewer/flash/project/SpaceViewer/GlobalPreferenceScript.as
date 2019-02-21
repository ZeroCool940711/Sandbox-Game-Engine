import com.adobe.serialization.json.JSON;

import flash.events.Event;
import flash.net.navigateToURL;

import mx.controls.Alert;
import mx.rpc.events.FaultEvent;

private var serviceURL:String;

// This array keeps track of all the changes made by user before pressing ok. 
// Objects in this array are of Iconsetting type.
private var iconarray:Array;
private var IsValidate:Boolean=false;

/*
 We make 3 concurrent http requests for data to fill the form. 
 As http responses take different time to arrive, we are using this variable to
 flag if all responses have arrived before setting the "enabled icon" checkbox and selecting
 the user selected icon. 
 */
private var preferencesSet:Boolean=false;
[Bindable]
private var entitynames:Array; //this variable is used to populate dropdown list

private function onResize():void
{
	try
	{
		tn.width = screen.width - 150;
		tn.height = screen.height - 100;
	}
	catch(error:Error)
	{
		
	}
}
/**
 * This function  selects the icon for selected entityType in entityTypeList 
 */	
public function setSavedPreferences():void
{
	var iconIterator:int=0;
	for(iconIterator=0;iconIterator < iconarray.length; iconIterator++)
	{
		//check for the selected entityType in entityList from the iconarray
		if(iconarray[iconIterator].entity_type == entity_list.selectedItem.toString())
		{
			//iconpath None string indicates icon is disabled
			if(iconarray[iconIterator].icon_path!="None")
			{
				iconChk.selected		= false;
				//search for the icon for that entityType from iconlist
				for(var iconIter:int=0 ; iconIter < myIconList.dataProvider.length ; iconIter++)
				{
					
					if(myIconList.dataProvider[iconIter].path.toString()==iconarray[iconIterator].icon_path.toString())
					{
						iconChk.selected		= true;
						myIconList.selectedIndex= iconIter;
						break;
					}
				}
			}
			else
			{
				iconChk.selected = false;
			}
			break;
		}
	}
	//if no field is present with that type means no icon selected
	if(iconIterator>=iconarray.length)
	{
		iconChk.selected = false;
	}
}
/**
 * Function for setting all radiobuttons false
 */
public function  setRadioButtonsFalse():void
{
	size25.selected		= false;
	size50.selected		= false;
	size75.selected		= false;
	size100.selected	= false;
	size150.selected	= false;
	CustomScale.selected= false;
}
/**
* This function consists of things required to be executed after getting globalpref
*/	
public function onGlobalPrefResult():void
{
	// Getting previously saved preferences
	var decoded:Object		= JSON.decode(getGPrefs.lastResult.toString());
	var type_array:Array	= decoded.entity_type;  //consists of entityTypes
	var path_array:Array	= decoded.entity_path;  //consists of iconpath for that type
	var entity_size:String	= decoded.entitysize;   //entity size previously set
	var EntitySize:Number	= 0.75;					//default if no saved entitysize
	if (entity_size)
		EntitySize = Number(entity_size);
	//setting the entity size option by current value
	if(EntitySize==0.25)
	{
		setRadioButtonsFalse();
		size25.selected		= true;
	}
	else if(EntitySize==0.50)
	{
		setRadioButtonsFalse();
		size50.selected		= true;
	}
	else if(EntitySize==0.75)
	{
		setRadioButtonsFalse();
		size75.selected  	= true;
	}
	else if(EntitySize==1.00)
	{
		setRadioButtonsFalse();
		size100.selected	= true;
	}
	else if(EntitySize==1.50)
	{
		setRadioButtonsFalse();
		size150.selected	= true;
	}
	else
	{
		setRadioButtonsFalse();
		CustomScale.selected= true;
		customValue.text	= (EntitySize*100).toString();
	}
	//create an entityType to iconpath mapping (IconSetting) to previously saved ones
	iconarray=new Array();
	for(var typeIterator:int=0;typeIterator<type_array.length;typeIterator++)
	{
		var temp:IconSetting = new IconSetting();
		temp.entity_type     = type_array[typeIterator].toString();
		temp.icon_path       = path_array[typeIterator].toString();
		iconarray.push(temp);
	}
	if(getnames.lastResult && catalogueForIcons.lastResult && !preferencesSet )
	{
		//setting the currently saved icon for entityType 
		preferencesSet=true;
		setSavedPreferences();
	}
}

 /**
 * This function consists of things to be done after getting entityType names list
 */
public function onNamesResult():void
{
	var decoded5:Object	= JSON.decode(getnames.lastResult.toString());
	entitynames			= decoded5.typeNames;
	// Check for the global pref result and icon list results are available
	//as all are httprequest it is hard to guess which one gets first		
	if(iconarray && catalogueForIcons.lastResult && !preferencesSet)
	{
		preferencesSet=true;
		setSavedPreferences();
	}
}

/**
 *  This function consists of things to be done when a user changes the entityType 
 * Combo Box
 */
public function onChangeCombo():void
{
	setSavedPreferences();
}

/**
 * This function serves as handler for both enableIcon checkBox and MyIconList Change 
 * 	saves the icon path for that entity Type selected in local reference iconarray 
 */
public function onChangeSelection():void
{
	try
	{
		if( catalogueForIcons.lastResult )
		{
			//Alert.show(myIconList.selectedItem.path.toString().charCodeAt(2));
			//search for the currently selected entityType and change the icon path with currently selected icon
			for(var iconIterator:int=0;iconIterator<iconarray.length;iconIterator++)
			{
				if(iconarray[iconIterator].entity_type == entity_list.selectedItem.toString())
				{
					if(iconChk.selected)
					{
						
						iconarray[iconIterator].icon_path	= myIconList.selectedItem.path.toString();
						return;
					}
					else
					{
						//if icon is disabled we are assigning "None"
						iconarray[iconIterator].icon_path = "None";
						return;
					}
				}
			}
			// There is no element present in iconnarray with that entityType create new one
			var temp:IconSetting=new IconSetting(); 
			temp.entity_type=entity_list.selectedItem.toString();
			
			if(iconChk.selected)
			{
				temp.icon_path 		= myIconList.selectedItem.path.toString();
			}
			else
			{
				//if icon is disabled we are assigning "None"
				temp.icon_path = "None";
			}
			iconarray.push(temp);
		}
	}
	catch(err:Error)
	{
		
	}
}

/**
 *  This function creates xml String using local icon image mapping reference i.e iconarray and 
 *  sends request to server for saving preferences at server
 */
public function saveIconSettings():void
{
	var xml_str:String = "<global>";
	var EntitySize:Number;
	if(size25.selected == true)
		EntitySize = 0.25;
	if(size50.selected == true)
		EntitySize = 0.50;
	if(size75.selected == true)
		EntitySize = 0.75;
	if(size100.selected == true)
		EntitySize = 1.00;
	if(size150.selected == true)
		EntitySize = 1.50;
	if(CustomScale.selected == true)
		EntitySize = Number(customValue.text)/100.0;
	xml_str += "<entitysize>" + EntitySize.toString() + "</entitysize>";
	for(var i:int=0;i<iconarray.length;i++)
	{
		xml_str += "<iconentity type=\""+iconarray[i].entity_type+"\" >";
		xml_str += iconarray[i].icon_path;
		xml_str += "</iconentity>";
	}
	xml_str += "</global>";
	var request:Object = new Object();
	request.xmlstr     = xml_str;
	setGPrefs.send(request);
}

public function onLoad():void
{
	currentState = "tab";
	tn.width = screen.width - 150;
	tn.height = screen.height - 100;
	try
	{
		//In this it gets the current preferences from server
		var request:Object = new Object();
		request.s		   = Math.random();
		serviceURL 		   = String(Application.application.parameters.serviceURL);
		catalogueForIcons.addEventListener(FaultEvent.FAULT,
					function (evt:FaultEvent):void
					{
						Alert.show("no Icons for icon Images are loaded ");
					}
		);
		catalogueForIcons.send(request);
		getnames.url = serviceURL+"/sv/getEntityNames?tg_format=json";
		getnames.send();
		getGPrefs.url = serviceURL+"/sv/getGlobalPref?tg_format=json";
		getGPrefs.send(request);
		setGPrefs.url = serviceURL+"/sv/saveGlobalPref?tg_format=json";
	}
	catch(error:Error)
	{
		Alert.show("error occurred while sending request for getting global preferences","Runtime error");
	}
}

public function onCancel():void
{
	navigateToURL(new URLRequest("/sv"),"_self");
}

/**
 * This function sends a request to save the current changed setting and navigate to the spaceviewer
 */
public function onClickOk():void
{
	IsValidate=onApply();
	if(IsValidate==true)
	{
		navigateToURL(new URLRequest("/sv"),"_self");
	}
}

/**
 * This function sends a request to save the current changed setting 
 */
public function onApply():Boolean
{
	if(CustomScale.selected==true && uint(customValue.text)<5)
	{
		Alert.show("Entity size must be more than 5");
		return false;
	}
	else if(CustomScale.selected==true && uint(customValue.text)>1000)
	{
		Alert.show("Entity size must be less than 1000");
		return false;
	}
	else
	{
		saveIconSettings();
		return true;
	}
}

/**
 * This function is performed on getting the result of request for catalogue.xml 
 * file. Sets the current selected icon. 
 */
public function onCatalogueResult():void
{
	// Check for the global pref result and entity type names results are available
	//as all are httprequest it is hard to guess which one gets first	
	if(iconarray && getnames.lastResult && !preferencesSet)
	{
		preferencesSet=true;
		setSavedPreferences();
	}
}
