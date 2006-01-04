function getLatLon(myForm) {
  iVolc = myForm.volc.selectedIndex
  numVolc = (arguments.length-1)/4
  myForm.lat.value = arguments[iVolc+1]
  myForm.lon.value = arguments[iVolc+1+numVolc]
  myForm.plumeMin.value = arguments[iVolc+1+numVolc+numVolc]
  myForm.area.value = arguments[iVolc+1+numVolc+numVolc+numVolc]
} // getLatLon

function makeUnknown(myForm,last) {
  myForm.volc.selectedIndex = last-1
} // makeUnknown

function checkEruptDate(myForm)
{
  if (myForm.eruptDate.value.length == 16)
  { 
    return;
  }
  alert("Bad eruption date specification; use YYYY MM DD HH:MM");
  myForm.eruptDate.value = ''
  return;
}
 
function adjustOptions(myForm) 
{
  // make eruptHours <= runHours
  if (parseFloat(myForm.runHours.value) < parseFloat(myForm.eruptHours.value))
  {
    myForm.eruptHours.value = myForm.runHours.value;
  }
  // make saveHours <= runHours
  if (parseFloat(myForm.runHours.value) < parseFloat(myForm.saveHours.value))
  {
    myForm.saveHours.value = myForm.runHours.value;
  }
  // allow only -phiDist _or_ -ashLogMean/-ashLogSdev
  if (myForm.phiDist.value.length > 0)
  {
    myForm.ashLogMean.value = '';
    myForm.ashLogSdev.value = '';
  }
  if (myForm.ashLogMean.value.length > 0)
  {
    myForm.phiDist.value = '';
  }
  return;
}
  
function seePrevious(runParamsFrame, ashxpFrame, sessionID) {
  dir = runParamsFrame.document.runParamsForm.previous.value
  runParamsFrame.location = "runParams.pl?previous=" + dir + "&sessionID=" + sessionID
  ashxpFrame.location = "ashxpOptions.pl?previous=" + dir + "&sessionID=" + sessionID
}

function loadjpeg(ashxpForm, imageFrame, workingDir) {
//  imageFrame.close()
  imageFrame.location = workingDir + ashxpForm.image.value
}

function loadmovie(imageFrame, movieFile) {
  imageFrame.location = movieFile
}

function openHelp(helpFile) {
  msgWindow=window.open
   (helpFile,"helpWindow","toolbar=no,width=500,height=200,scrollbars=yes")
   }

function resetImage(imageFrame) {
  imageFrame.location = 'pix/puff_bg.gif'
  }

function openMapOptions() {
  msgWindow=window.open("mapOptions.pl")
}
  
function previousDefault(ashxpFrame, sessionID, previous) {
  ashxpFrame.location = "ashxpOptions.pl?previous=" + previous + "&sessionID=" + sessionID
}
