var admin = require("firebase-admin");

var serviceAccount = require("../FCM_Key.json");

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount)
});

const sendNotificationToMulti = async (registrationTokens, title, body) => {
  if (!registrationTokens || registrationTokens.length === 0) {
    console.log("No registration tokens found. Skipping notification.");
    return false;
  }

  // notification 是專門用來發送通知的，而 data 是用來傳遞自定義數據的 (手機程式在執行時使用)
  // https://firebase.google.com/docs/cloud-messaging/concept-options?hl=zh-tw
  const message = {
    notification: {
      'title': title,
      'body': body,
    },
    android: {
      notification: {
        channelId: 'High Importance Notification' 
      }
    },
    tokens: registrationTokens
  }
  try {
    const response = await admin.messaging().sendEachForMulticast(message);
    if (response.failureCount > 0) {
      const failedTokens = [];
      response.responses.forEach((resp, idx) => {
        if (!resp.success) {
          failedTokens.push(registrationTokens[idx]);
        }
      });
      console.log('List of tokens that caused failures: ' + failedTokens);
      return false; // Indicate failure
    } else {
      console.log('All notifications sent successfully to FCM');
      return true; // Indicate success
    }
  } catch (error) {
    console.error('Error sending notification:', error);
    return false; // Indicate failure on error
  }
}
module.exports = {
  sendNotificationToMulti
};