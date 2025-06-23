# README.MD  

This repository provides a guide to the decryption process and associated code sample to help you add decryption of a Recall user's exported snapshots to your app.

Additional documentation for this sample app and the decryption process can be found [here](https://learn.microsoft.com/windows/ai/recall/decrypt-exported-snapshots). 


## Decrypting Snapshots

To decrypt exported snapshots from Recall, ensure the following:

- The user must first [Export Recall snapshots](https://support.microsoft.com/topic/680bd134-4aaa-4bf5-8548-a8e2911c8069) using the Recall export code that they were provided during the Recall First Run Experience. You will need the location (folder path) containing the user's previously exported (encrypted) snapshots.
- You will need the user to provide the 32-character Recall export code associated with their snapshots in order to decrypt those snapshots.
- You will need to define the location (folder path) where the decrypted .jpg and .json files associated with the user's exported snapshots will be saved.
- The sample shows code that takes the above information and decrypts the provided snapshots.


## Building the sample

1. Open RecallSnapshotsExport.sln in Visual Studio 2022
2. Select your build target (e.g. Release, x64)
3. Select Project, Build Solution