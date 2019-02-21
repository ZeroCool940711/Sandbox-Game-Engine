import mx.controls.Alert;
import flash.events.*;
import myComponents.Thumbnail;
private var index:uint=0;
private var indexForIcon:uint=0;

// OK button click handler for Configuration Screen
public function onClickOk():void
{
   	if(Number(CellAppUpdateFreq.text)<0.25 && Number(CellAppManagerUpdateFreq.text)<0.25 )
   	{	
   		Alert.show("Frequencies must be more than 0.25 seconds");
   		CellAppUpdateFreq.text = "0.25";
   		CellAppManagerUpdateFreq.text = "0.25";
   	}
   	else if(Number(CellAppUpdateFreq.text)<0.25)
   	{
   		Alert.show("Cell App Update Frequency must be more than 0.25 seconds");
   		CellAppUpdateFreq.text = "0.25";
   	}
   	else if(Number(CellAppManagerUpdateFreq.text)<0.25)
   	{
   		Alert.show("Cell App Manager Update Frequency must be more than 0.25 seconds");
   		CellAppManagerUpdateFreq.text = "0.25";
   	}
   	else
   	{
   		onOKColour();
       	onOKImageOverlay();
		onOKView();
		currentState		=	'';
		var request:Object	=	new Object();
		/*request.entitysize	=	config.EntitySize*100;
		request.imageoverlay=	String(config.IsImageOverlay);
		if(imgChk.selected)
		{
			request.imagepath	=	String(config.ImagePath);
		}            	
		setPrefs.send(request);*/
	}
}
	
// Cancel button click handler for Configuration Screen
public function onCancel():void
{
	currentState = '';
}
	
// Function for setting all the parameters related to Colour tab in Configuration Screen
public function onOKColour():void
{
	config.CellAppIDColour			=	CellAppID.selectedColor;
	config.IPAddressColour			=	IPAddress.selectedColor;
	config.CellLoadColour			=	CellLoad.selectedColor;
	config.IsPartitionLoadEnabled	=	PartitionLoadEnable.selected;
	if(PartitionLoadEnable.selected)
		config.PartitionLoadColour	=	PartitionLoad.selectedColor;
	config.IsEntityBoundsEnabled	=	EntityBoundsEnable.selected;
	if(EntityBoundsEnable.selected)
		config.EntityBoundsColour	=	EntityBounds.selectedColor;					
	config.SpaceBoundsColour		=	SpaceBounds.selectedColor;
	config.GridColour				=	Grid.selectedColor;
	config.GhostEntityColour		=	GhostEntity.selectedColor;
	config.CellBoundaryColour		=	CellBoundary.selectedColor;
	if(RelativeColour.selected == true)
		config.IsRelativeColour		=	true;
	else
		config.IsRelativeColour		=	false;
}
	
// Function for setting all the parameters related to View tab in Configuration Screen
public function onOKView():void
{
	config.CellAppUpdateFrequency 			= Number(CellAppUpdateFreq.text);
	config.CellAppManagerUpdateFrequency 	= Number(CellAppManagerUpdateFreq.text);
}
	
// Function for setting all the parameters related to Image Overlay tab in Configuration Screen
public function onOKImageOverlay():void
{
	var xmlStr:String="";
	xmlStr+="<root>";
	for(var key:String in config.imageSpaceDict)
	{
		xmlStr+="<imageOverlay name=\""+key+
				"\">"+config.imageSpaceDict[key]+"</imageOverlay>";
	}
	/*for(var i:int=0;i<config.imageOverlayArray.length;i++)
	{
		xmlStr+="<imageOverlay name=\""+config.imageOverlayArray[i].spaceName+
				"\">"+config.imageOverlayArray[i].imagePath+"</imageOverlay>";
	}*/
	xmlStr+="</root>";
	var request:Object=new Object();
	request.xmlStr=xmlStr;
	setPrefs.send(request);	
	/*if(imgChk.selected==true)
	{
		index					=	myList.selectedIndex;
		config.IsImageOverlay	=	true;
		config.ImagePath		=	myList.selectedItem.path.toString();
	}
	else
	{
		config.IsImageOverlay	=	false;
	}*/
}
	
// Config button click handler for the Main Screen for restoring all the set values related to Configuration Screen 
private function configClick():void
{
	var req:Object					=	new Object();
	var num:Number					=	Math.random();
	req.ran							=	num;
	catalogue.send(req);
	CellAppID.selectedColor			=	config.CellAppIDColour;
	IPAddress.selectedColor			=	config.IPAddressColour;
	CellLoad.selectedColor			=	config.CellLoadColour;
	PartitionLoadEnable.selected	=	config.IsPartitionLoadEnabled;
	if(config.IsPartitionLoadEnabled)
		PartitionLoad.selectedColor	=	config.PartitionLoadColour;
	EntityBoundsEnable.selected		=	config.IsEntityBoundsEnabled;
	if(config.IsEntityBoundsEnabled)
		EntityBounds.selectedColor	=	config.EntityBoundsColour;
	SpaceBounds.selectedColor		=	config.SpaceBoundsColour;
	Grid.selectedColor				=	config.GridColour;
	GhostEntity.selectedColor		=	config.GhostEntityColour;
	CellBoundary.selectedColor		=	config.CellBoundaryColour;
	RelativeColour.selected			=	config.IsRelativeColour;
	CellAppUpdateFreq.text			=	config.CellAppUpdateFrequency.toString();
	CellAppManagerUpdateFreq.text	=	config.CellAppManagerUpdateFrequency.toString();
	if(imgChk.selected==true)
	{
		myList.selectedIndex	=	index;
	}
	imgChk.selected		=	config.IsImageOverlay;
	tn.selectedIndex	=	0;
	
	//Releasing the memory
	if(req!=null)
	{
		delete req.ran;
	}
	req = null;
	//Showing the config screen according to browser size
	ConfigPanel.width = screen.width - 30;
	ConfigPanel.height = screen.height - 80;
}	
public function setCurrentSettings():void
{
	var res:String=config.imageSpaceDict[SpaceCmb.selectedItem.toString()];
	if(res==null || res=="None")
	{
		imgChk.selected=false;
		return;
	}
	for(var j:int=0 ; j < myList.dataProvider.length ; j++)
	{
		
		if(myList.dataProvider[j].path.toString()==res)
		{
			imgChk.selected		= true;
			myList.selectedIndex= j;
			break;
		}
	}
}
public function onChangeSpaceCmb():void
{
	if(catalogue.lastResult)
	{
		setCurrentSettings();
	}
}	
public function onCatalogueResult():void
{
	if(currentState=="config")
		setCurrentSettings();
}
public function onChangeImageList():void
{
	saveImageSetting();

}
public function saveImageSetting():void
{
	try
	{
	if(imgChk.selected)
	{
		
		config.imageSpaceDict[SpaceCmb.selectedItem.toString()]	= myList.selectedItem.path.toString();
		return;
	}
	else
	{
		//if image is disabled we are assigning "None"
		config.imageSpaceDict[SpaceCmb.selectedItem.toString()]	= "None";
		return;
	}
	}catch(err:Error)
	{
		
	}

}
public function onChangeImgChk():void
{
	if(catalogue.lastResult)
		saveImageSetting();
}