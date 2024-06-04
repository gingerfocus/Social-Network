#include "socialnetworkwindow.h"
#include "ui_socialnetworkwindow.h"

SocialNetworkWindow::SocialNetworkWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::SocialNetworkWindow)
{
    ui->setupUi(this);

    // initially hide everything that isn't login page
    ui->invalidName->setVisible(false);
    ui->personProfile->setVisible(false);
    ui->recentPosts->setVisible(false);
    ui->returnHome->setVisible(false);
    ui->userFriends->setVisible(false);
    ui->friendSuggestions->setVisible(false);
    ui->friendSuggestionsLabel->setVisible(false);
    ui->addFriend->setVisible(false);

    // read in users and posts
    n.readUsers("C:\\Qt\\workspace\\Class\\HW4\\users.txt");
    n.readPosts("C:\\Qt\\workspace\\Class\\HW4\\posts.txt");

    // connect everything
    connect(ui->loginButton, &QPushButton::clicked, this, &SocialNetworkWindow::login);
    connect(ui->returnHome, &QPushButton::clicked, this, &SocialNetworkWindow::goToHome);
    connect(ui->userFriends, &QTableWidget::cellClicked, this, &SocialNetworkWindow::goToFriend);
    connect(ui->friendSuggestions, &QTableWidget::cellClicked, this, &SocialNetworkWindow::goToFriendSuggestion);
    connect(ui->addFriend, &QPushButton::clicked, this, &SocialNetworkWindow::addAsFriend);
}

void SocialNetworkWindow::login()
{
    QString qname = ui->loginTextBox->toPlainText();
    std::string name = qname.toStdString();

    // if name is not already in here, it is invalid
    int id = n.getId(name);
    if (id == -1)
    {
        ui->invalidName->setVisible(true);
        return;
    }
    else
    {
        // hide everything on the login page
        ui->enterNameLabel->setVisible(false);
        ui->loginTextBox->setVisible(false);
        ui->loginButton->setVisible(false);
        ui->invalidName->setVisible(false);

        // show everything in profile page
        ui->personProfile->setVisible(true);
        ui->recentPosts->setVisible(true);
        ui->userFriends->setVisible(true);
        ui->friendSuggestions->setVisible(true);
        ui->friendSuggestionsLabel->setVisible(true);

        // initialize the most recent and initial users
        User *u = n.getUser(id);
        initialUser = u;
        mostRecentUser = u;

        // make table full of user friends
        std::set<int> uFriends = u->getFriends();
        int numberFriends = uFriends.size();
        ui->userFriends->setRowCount(numberFriends);
        ui->userFriends->setColumnCount(1);

        int i = 0;
        for (auto itr = uFriends.begin(); itr != uFriends.end(); ++itr)
        {
            int tempFriendID = *itr;
            User *tempFriend = n.getUser(tempFriendID);
            std::string name = tempFriend->getName();
            QString qname = QString::fromStdString(name);
            QTableWidgetItem *displayedName = new QTableWidgetItem(qname);
            ui->userFriends->setItem(i, 0, displayedName);

            ++i;
        }

        // make table for friend suggestions
        int score = 0;
        std::vector<int> friendSuggestions = n.suggestFriends(id, score);
        int friendSuggestionsSize = friendSuggestions.size();
        ui->friendSuggestions->setRowCount(friendSuggestionsSize);
        ui->friendSuggestions->setColumnCount(1);

        for (int i = 0; i < friendSuggestionsSize; ++i)
        {
            int friendSuggestionId = friendSuggestions[i];
            User *friendSuggestion = n.getUser(friendSuggestionId);
            std::string name = friendSuggestion->getName();
            QString qname = QString::fromStdString(name);
            QTableWidgetItem *displayedName = new QTableWidgetItem(qname);
            ui->friendSuggestions->setItem(i, 0, displayedName);
        }

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // make posts too(FOR TABLE)
        int totalPosts = 0;
        std::set<int> friends = u->getFriends();
        for (int friendId : friends)
        {
            User *friendUser = n.getUser(friendId);
            totalPosts += friendUser->getPosts().size();
        }

        // Set the row count to the total number of posts
        ui->postsTable->setRowCount(totalPosts);

        ui->postsTable->setColumnCount(4);

        // Set the width of the columns
        ui->postsTable->setColumnWidth(0, 200); // adjust as needed
        ui->postsTable->setColumnWidth(1, 700); // adjust as needed
        ui->postsTable->setColumnWidth(2, 100); // adjust as needed
        ui->postsTable->setColumnWidth(3, 100); // adjust as needed

        // Initialize a counter for the current row
        int currentRow = 0;

        // Iterate over all friends
        for (int friendId : friends)
        {
            User *friendUser = n.getUser(friendId);

            // Get all posts from the current friend
            std::vector<Post *> posts = friendUser->getPosts();

            // Get all posts as a string
            std::string postsString = friendUser->getPostsString(posts.size(), false);

            // Split the posts string into individual posts
            QStringList individualPosts = QString::fromStdString(postsString).split("\n\n");

            // Iterate over the individualPosts list and insert each post's text into the table
            for (int i = 0; i < individualPosts.size(); ++i)
            {
                const QString &post = individualPosts[i];
                Post *postObj = posts[individualPosts.size() - i - 1]; // since individualPosts is constructed backwards

                // Create a new QTableWidgetItem with the friend's name
                QTableWidgetItem *nameItem = new QTableWidgetItem(QString::fromStdString(friendUser->getName()));

                // Insert the name item into the table at row currentRow, column 0
                ui->postsTable->setItem(currentRow, 0, nameItem);

                // Create a new QTableWidgetItem with the post's text
                QTableWidgetItem *postItem = new QTableWidgetItem(post);

                // Insert the post item into the table at row currentRow, column 1
                ui->postsTable->setItem(currentRow, 1, postItem);

                // Create a new QPushButton with the text "Like Post"
                QPushButton *button = new QPushButton("Like Post");

                // Connect the button's clicked signal to the likePost slot function
                connect(button, &QPushButton::clicked, this, [this, currentRow, userId = u->getId()]()
                        {
                    // Get the messageId from the table
                    int messageId = ui->postsTable->item(currentRow, 3)->text().toInt();

                    // Call the likePost function with the messageId and userId
                    likePost(currentRow, messageId, userId); });

                // Insert the button into the table at row currentRow, column 2
                ui->postsTable->setCellWidget(currentRow, 2, button);

                // Create a new QTableWidgetItem with the post's messageId
                QTableWidgetItem *messageIdItem = new QTableWidgetItem(QString::number(postObj->getMessageId()));

                // Insert the messageId item into the table at row currentRow, column 3
                ui->postsTable->setItem(currentRow, 3, messageIdItem);

                // Increment the current row counter
                ++currentRow;
            }
        }

        // FOR TEXT BOX, NOT TABLE
        // std::vector<Post*> posts = u->getPosts();
        // int numPosts = posts.size();
        // numPosts = std::min(numPosts, 5);
        // std::string allPosts = u->getPostsString(numPosts, false);
        // QString qAllPosts = QString::fromStdString(allPosts);
        // ui->recentPosts->setText(qAllPosts);

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
}

void SocialNetworkWindow::goToHome()
{
    // change the mostRecentId to initial user
    mostRecentUser = initialUser;

    // read in users, it might have changed
    n.readUsers("C:\\Qt\\workspace\\Class\\HW4\\users.txt");

    // have to show and hide stuff
    ui->returnHome->setVisible(false);
    ui->friendSuggestions->setVisible(true);
    ui->friendSuggestionsLabel->setVisible(true);
    ui->addFriend->setVisible(false);

    // change name
    ui->personProfile->setText("My Profile");

    // change friend list
    std::set<int> uFriends = initialUser->getFriends();
    int numberFriends = uFriends.size();
    ui->userFriends->setRowCount(numberFriends);
    ui->userFriends->setColumnCount(1);

    int i = 0;
    for (auto itr = uFriends.begin(); itr != uFriends.end(); ++itr)
    {
        int tempFriendID = *itr;
        User *tempFriend = n.getUser(tempFriendID);
        std::string name = tempFriend->getName();
        QString qname = QString::fromStdString(name);
        QTableWidgetItem *displayedName = new QTableWidgetItem(qname);
        ui->userFriends->setItem(i, 0, displayedName);

        ++i;
    }

    // // change posts
    // std::vector<Post *> posts = initialUser->getPosts();
    // int numPosts = posts.size();
    // numPosts = std::min(numPosts, 5);
    // std::string allPosts = initialUser->getPostsString(numPosts, false);
    // QString qAllPosts = QString::fromStdString(allPosts);
    // ui->recentPosts->setText(qAllPosts);
}

void SocialNetworkWindow::goToFriend(int row, int column)
{
    // hide table of friend suggestions and show home button
    ui->friendSuggestions->setVisible(false);
    ui->friendSuggestionsLabel->setVisible(false);
    ui->returnHome->setVisible(true);

    // get name of clicked person, set profile name to that
    QTableWidgetItem *item = ui->userFriends->item(row, column);
    QString qItemName = item->text();
    ui->personProfile->setText(qItemName + "'s profile");

    // change the name of the add friend label
    ui->addFriend->setText("Add " + qItemName + " as a friend");
    ui->addFriend->setVisible(true);

    // change most recent user to the friend
    std::string friendName = qItemName.toStdString();
    int uFriendId = n.getId(friendName);
    User *uFriend = n.getUser(uFriendId);
    mostRecentUser = uFriend;

    // change the friend list to friends of the friend, and add the friend to the stack
    std::set<int> uFriendsFriends = uFriend->getFriends();
    int numberFriends = uFriendsFriends.size();
    ui->userFriends->setRowCount(numberFriends);
    ui->userFriends->setColumnCount(1);

    int i = 0;
    for (auto itr = uFriendsFriends.begin(); itr != uFriendsFriends.end(); ++itr)
    {
        int tempFriendID = *itr;
        User *tempFriend = n.getUser(tempFriendID);
        std::string name = tempFriend->getName();
        QString qname = QString::fromStdString(name);
        QTableWidgetItem *displayedName = new QTableWidgetItem(qname);
        ui->userFriends->setItem(i, 0, displayedName);

        ++i;
    }

    // // make posts too, set visibility of incoming posts to private
    // std::vector<Post *> posts = uFriend->getPosts();
    // int numPosts = posts.size();
    // numPosts = std::min(numPosts, 5);
    // std::string allPosts = uFriend->getPostsString(numPosts, true);
    // QString qAllPosts = QString::fromStdString(allPosts);
    // ui->recentPosts->setText(qAllPosts);
}

void SocialNetworkWindow::goToFriendSuggestion(int row, int column)
{
    // hide table of friend suggestions and show home button
    ui->friendSuggestions->setVisible(false);
    ui->friendSuggestionsLabel->setVisible(false);
    ui->returnHome->setVisible(true);

    // get name of clicked person, set profile name to that
    QTableWidgetItem *item = ui->friendSuggestions->item(row, column);
    QString qItemName = item->text();
    ui->personProfile->setText(qItemName + "'s profile");

    // change the name of the add friend label
    ui->addFriend->setText("Add " + qItemName + " as a friend");
    ui->addFriend->setVisible(true);

    // change most recent user to the friend
    std::string friendName = qItemName.toStdString();
    int uFriendId = n.getId(friendName);
    User *uFriend = n.getUser(uFriendId);
    mostRecentUser = uFriend;

    // change the friend list to friends of the friend, and add the friend to the stack
    std::set<int> uFriendsFriends = uFriend->getFriends();
    int numberFriends = uFriendsFriends.size();
    ui->userFriends->setRowCount(numberFriends);
    ui->userFriends->setColumnCount(1);

    int i = 0;
    for (auto itr = uFriendsFriends.begin(); itr != uFriendsFriends.end(); ++itr)
    {
        int tempFriendID = *itr;
        User *tempFriend = n.getUser(tempFriendID);
        std::string name = tempFriend->getName();
        QString qname = QString::fromStdString(name);
        QTableWidgetItem *displayedName = new QTableWidgetItem(qname);
        ui->userFriends->setItem(i, 0, displayedName);

        ++i;
    }

    // // make posts too, set visibility of incoming posts to private
    // std::vector<Post *> posts = uFriend->getPosts();
    // int numPosts = posts.size();
    // numPosts = std::min(numPosts, 5);
    // std::string allPosts = uFriend->getPostsString(numPosts, true);
    // QString qAllPosts = QString::fromStdString(allPosts);
    // ui->recentPosts->setText(qAllPosts);
}

void SocialNetworkWindow::addAsFriend()
{
    // get names of original user and the clicked on user
    std::string originalUName = initialUser->getName();
    std::string recentUName = mostRecentUser->getName();

    // add connection
    n.addConnection(originalUName, recentUName);

    // change the txt file of user and post
    n.writeUsers("C:\\Qt\\workspace\\Class\\HW4\\users.txt");
    n.writePosts("C:\\Qt\\workspace\\Class\\HW4\\posts.txt");

    // remake friend suggestions based on this new connection
    int score = 0;
    std::vector<int> friendSuggestions = n.suggestFriends(initialUser->getId(), score);
    int friendSuggestionsSize = friendSuggestions.size();
    ui->friendSuggestions->setRowCount(friendSuggestionsSize);
    ui->friendSuggestions->setColumnCount(1);

    for (int i = 0; i < friendSuggestionsSize; ++i)
    {
        int friendSuggestionId = friendSuggestions[i];
        User *friendSuggestion = n.getUser(friendSuggestionId);
        std::string name = friendSuggestion->getName();
        QString qname = QString::fromStdString(name);
        QTableWidgetItem *displayedName = new QTableWidgetItem(qname);
        ui->friendSuggestions->setItem(i, 0, displayedName);
    }
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void SocialNetworkWindow::likePost(int row, int messageId, int userId)
{
    Post *post = n.getPost(messageId);

    // Increase the number of likes for the post
    post->like(userId);

    // Update the table to reflect the new number of likes
    ui->postsTable->item(row, 1)->setText(QString::fromStdString(post->toString()));

    // Also update posts.txt
    n.writePosts("C:\\Qt\\workspace\\Class\\HW4\\posts.txt");
}
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SocialNetworkWindow::~SocialNetworkWindow()
{
    delete ui;
}
