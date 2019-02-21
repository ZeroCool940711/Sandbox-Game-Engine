<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml"
      xmlns:py="http://purl.org/kid/ns#"
      py:layout="'../../common/templates/layout.kid'">
      <!--py:extends="'../../common/templates/common.kid'">-->

<div py:def="moduleContent()" py:strip="True">
  <head>	
  </head>
<body  >
<div style=" background-image:url('static/images/Back.bmp'); ">

<script type="text/javascript">
	PAGE_TITLE = 'Upload Image';

	function onSub()
	{
	
	if(document.forms[0].elements[0].checked||document.forms[0].elements[1].checked)
	{
	if(document.forms[0].elements[2].value!="")
	{
	var fname=document.forms[0].elements[2].value;	
    temp = fname.split('.');
    len = temp.length;
    //then the file extension will be in temp[len-1]....
    //if u want to compare just u give like this
    if(temp[len-1].toLowerCase() == 'gif'||temp[len-1].toLowerCase() == 'jpeg'||temp[len-1].toLowerCase() == 'png'||temp[len-1].toLowerCase() == 'jpg'){
	return true
    }
	else
	{
	alert("Invalid Format. You can upload only *.gif, *.jpeg, *.jpg, *.png files");
	return false;
	}
	
	;
	}
	else
	{
	alert("Select an image");
	return false;
	
	}
	
	}
	else
	{
	alert("Choose any option");
	return false;
	}
	}
		
	
	</script>


    <P style="font-size:large; color:rgb(255,255,255);" > &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Upload your image </P>
    <P>&nbsp;</P>
   <FORM METHOD="post" name="form1"  action="uploadFile" enctype="multipart/form-data" onSubmit="return onSub()">
    
     <P>&nbsp;</P>
     <P>&nbsp;</P>
     <P style="color:rgb(255,255,255);"> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;


       <INPUT TYPE="radio" id="r1" NAME="IsImage"  checked="true" value="image" style="font-style:normal; font-size:medium;">Upload an Image For Image Overlay</INPUT>
       <br/>
     &nbsp;</P>
     <P style="color:rgb(255,255,255);"> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;


       <INPUT TYPE="radio" id="r2" NAME="IsImage" value="icon">Upload an Icon For Displaying Entity</INPUT>
       <br/>
     </P>
     <P>&nbsp;</P>
     <P>&nbsp;</P>
     <P>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;


       <INPUT TYPE="file" NAME="upload_file" style="background-color:rgb(255,255,255);  border-style:none; ">
	   </INPUT>
     </P>
     <P>&nbsp;</P>
     <P>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;


     <INPUT TYPE="submit" value="Upload" >
	 </INPUT></P>
      <P>&nbsp;</P>
     <P>&nbsp;</P>
     <P>&nbsp;</P>
     <P>&nbsp;</P>
     <P>&nbsp;</P>
  </FORM>
</div>
  </body>


  
</div>
</html>
