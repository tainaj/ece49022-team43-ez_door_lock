# ECE 49022 Senior Design Team 43: EZ Door Lock
A ECE 49022 senior design project to be submitted to College of Electrical and Computer Engineering, Purdue University. This is a collaborative effort by authors Sung-Woo Jang, Benjamin Oh, and Joel Taina. Each major subsystem was designed by the corresponding author.

The EZ Door Lock is an electronic access control system, to be installed on doors, that adopts
fingerprint recognition to provide speedy, personalized authorization for users.

## Synopsis
The EZ Door Lock is an electronic access control system, to be installed on doors, that adopts
fingerprint recognition to provide speedy, personalized authorization for users.

### Mode of operation
EZ Door Lock will detect and evaluate a finger pattern in less than a
second, providing speedy entry for allowed users. Upon a successful entry, the door will unlock;
it will lock automatically once the door is shut. PIN entry is used both as a means to access
manually as well as access to admin control, which allows admins to add and remove profiles.

### Major subsystems
EZ Door Lock is controlled by the ESP32-WROOM-32 Wi-Fi module. It consists of the following subsystems:
* Profile Recognition
* User Interface and Interaction of Things
* Wireless Communication

## r

The user interface
(UI) consists of the following: a fingerprint scanner, a telephone keypad, and an LCD text display.
Once a finger pattern is detected by the scanner, the system scans its database of up to 16 user
profiles to find a match. EZ Door Lock will detect and evaluate a finger pattern in less than a
second, providing speedy entry for allowed users. Upon a successful entry, the door will unlock;
it will lock automatically once the door is shut. PIN entry is used both as a means to access
manually as well as access to admin control, which allows admins to add and remove profiles.

**A Github platform for ECE 49022 Team 43 work**

## Links
https://engineering.purdue.edu/~ee469/ -> ECE 469 home page

## Common git commands:
- git checkout *my_branch*                      -> switch to different local branch
- git commit -m "*message*" *file1* *file2* *etc* -> commit select files
- git pull *partnered* *master*                -> pull content from remote branch (<remote_name>/master)
- git push *remote_name* *branch_name*     -> push content to remote branch
- git remote -v                        -> view mapping of remote branch nickname to URL
- git branch -a                        -> view all branches, including remote branches
- git diff *master* *partnered/master*         -> compare commits between local branch and remote branch
- git help *command*                     -> opens help guide for command

## How to create/edit files to upload to Git
1. Create namesake files in local repository (functionality not important, this is so push/pull can work without merge conflicts)
2. Commit files
3. Push
4. Pull