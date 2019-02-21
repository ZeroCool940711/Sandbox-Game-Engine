function onSelectMachines( elt )
{
	var singleRadioButton = document.getElementById( "ccStartMachineMode" );
	var groupRadioButton = document.getElementById( "ccStartGroupMode" );
	var layoutRadioButton = document.getElementById( "ccStartLayoutMode" );

	// If no form element passed, work out which one to use from the radio button
	if (elt == null)
	{
		if (singleRadioButton.checked)
			elt = document.startForm.machine;
		else if (groupRadioButton.checked)
			elt = document.startForm.group;
		else if (layoutRadioButton.checked)
			elt = document.startForm.layout;
		else
		{
			Util.error( "No radio button selected!" );
			return;
		}
	}

	var restrictCheckBox =  document.startForm.restrict;
	restrictCheckBox.disabled = true;

	var radiobutton;
	if (elt.name == "machine")
		radiobutton = singleRadioButton;
	else if (elt.name == "layout")
		radiobutton = layoutRadioButton;
	else if (elt.name == "group")
	{
		radiobutton = groupRadioButton;
		restrictCheckBox.disabled = false;
	}
	else
	{
		Util.error( "onSelectMachines:: no radiobutton for " + elt.name );
		return;
	}

	radiobutton.checked = true;
	verifyEnv( elt );
}

function verifyEnv( elt )
{
	var dict = new Object();
	dict[ "user" ] = username;
	dict[ "type" ] = elt.name;
	dict[ "value" ] = elt.value;

	var onSuccess = function( dict )
	{
		setEnv( dict.mfroot, dict.bwrespath );
		document.getElementById( "ccStartSubmit" ).disabled = false;
	};

	var onError = function( error, details )
	{
		setEnv( "<error>", "<error>" );
		Util.error( error, details );
		document.getElementById( "ccStartSubmit" ).disabled = true;
	};

	Ajax.call( "/cc/verifyEnv", dict, onSuccess, onError );
}

function setEnv( mfroot, bwrespath )
{
	$("ccStartMFRoot").innerHTML = mfroot;
	$("ccStartBWResPath").innerHTML = bwrespath;
}
