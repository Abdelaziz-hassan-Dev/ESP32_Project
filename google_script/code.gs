/**
 * Entry point for the Web App.
 * Handles HTTP GET requests from the ESP32 and logs sensor data to the spreadsheet.
 */
function doGet(e) { 
  // Log incoming request parameters for debugging
  Logger.log(JSON.stringify(e));

  var result = 'Ok';
  
  // Validate that parameters exist
  if (e.parameter == 'undefined') {
    result = 'No Parameters';
  } else {
    // --- 1. Spreadsheet Configuration ---
    var sheet_id = 'YOUR_SPREADSHEET_ID_HERE';
    var sheet = SpreadsheetApp.openById(sheet_id).getActiveSheet();
    
    // --- 2. Prepare New Row Data ---
    var newRow = sheet.getLastRow() + 1; 
    var rowData = []; 
    
    // Generate timestamps relative to the project location (Istanbul)
    var currDate = Utilities.formatDate(new Date(), "Europe/Istanbul", "yyyy-MM-dd");
    var currTime = Utilities.formatDate(new Date(), "Europe/Istanbul", "HH:mm:ss");
    
    rowData[0] = currDate; // Column A: Date
    rowData[1] = currTime; // Column B: Time
    
    // --- 3. Map Parameters to Columns ---
    for (var param in e.parameter) {
      // Clean up input value
      var value = stripQuotes(e.parameter[param]);
      
      // Map specific parameters to their designated columns
      switch (param) {
        case 'temp':
          rowData[2] = value; // Column C
          break;
        case 'hum':
          rowData[3] = value; // Column D
          break;
        case 'fire':
          rowData[4] = value; // Column E
          break;
      }
    }
    
    // --- 4. Write Data ---
    // Update the sheet in a single batch operation for efficiency
    var newRange = sheet.getRange(newRow, 1, 1, rowData.length);
    newRange.setValues([rowData]);
  }
  
  // Return simple text response to the client (ESP32)
  return ContentService.createTextOutput(result);
}

/**
 * Utility: Removes surrounding double quotes from a string if present.
 * Prevents formatting issues if the sender includes extra quotes.
 */
function stripQuotes(a) {
  if (a.charAt(0) === '"' && a.charAt(a.length-1) === '"') {
    return a.substr(1, a.length-2);
  }
  return a;
}