/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2015 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

function guid4Block() {
  return Math.floor((1 + Math.random()) * 0x10000)
    .toString(16)
    .substring(1);
}
 
function guid() {
  return (guid4Block() + guid4Block() + '-' + guid4Block() + '-' + guid4Block() + '-' +
          guid4Block() + '-' + guid4Block() + guid4Block() + guid4Block());
}


$(document).ready(function() {
  $('#patientID').val(guid());

  $('#submit').click(function(event) {
    var png = context.canvas.toDataURL();

    $.ajax({
      type: 'POST',
      url: '/orthanc/tools/create-dicom',
      dataType: 'text',
      data: { 
        PatientID: $('#patientID').val(),
        PatientName: $('#patientName').val(),
        StudyDescription: $('#studyDescription').val(),
        SeriesDescription: $('#seriesDescription').val(),
        PixelData: png,
        Modality: 'RX'
      },
      success : function(msg) {
        alert('Your drawing has been DICOM-ized!\n\n' + msg);
      },
      error : function() {
        alert('Error while DICOM-izing the drawing');
      }
    });

    return false;
  });
});
