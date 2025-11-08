# Hunt Master Academy - Mobile Platform User Stories

**Last Updated:** November 8, 2025  
**Status:** Requirements Specification  
**Platforms:** iOS 14+ | Android 9+ (API 28+)  
**Related Docs:** `MICROSERVICES_ARCHITECTURE_GUIDE.md`, `phase3_ai-coach_todo.md`

---

## Document Purpose

This document defines comprehensive user stories for Hunt Master Academy Game Calls mobile applications (iOS and Android). These stories guide feature development, UX design, and platform-specific implementation from initial app download through advanced gameplay features.

**Target Users:**
- Novice hunters learning animal calling techniques
- Intermediate hunters refining their skills
- Expert hunters perfecting specific call types
- Hunting instructors teaching calling techniques

---

## Platform-Specific Considerations

### iOS Platform Requirements
- **Minimum Version:** iOS 14.0+
- **Target Devices:** iPhone 8 and newer, iPad (6th gen+)
- **Key Features:**
  - Native SwiftUI or React Native interface
  - AVFoundation for audio recording
  - App Store authentication
  - iCloud sync for cross-device progress
  - Push notifications via APNs
  - Haptic feedback for user actions

### Android Platform Requirements
- **Minimum Version:** Android 9.0 (Pie, API 28+)
- **Target Devices:** Mid-to-high tier smartphones with 3GB+ RAM
- **Key Features:**
  - Native Kotlin or React Native interface
  - MediaRecorder API for audio capture
  - Google Play authentication
  - Firebase sync for cross-device progress
  - Push notifications via FCM
  - Vibration feedback for user actions

---

## Epic 1: App Download & Installation

### User Story 1.1: App Discovery
**As a** prospective user  
**I want to** discover the Hunt Master Academy app in the App Store/Play Store  
**So that** I can download and install it on my device

**Acceptance Criteria:**
- [ ] App appears in search results for keywords: "hunting calls", "game calls", "deer calls", "turkey calls", "hunt training"
- [ ] App listing displays clear description, screenshots, and feature highlights
- [ ] App icon is professional and recognizable (green/camo theme)
- [ ] App rating and review count are visible
- [ ] App size and requirements are clearly stated
- [ ] Preview video shows core functionality (30-60 seconds)

**Platform Notes:**
- **iOS:** App Store listing with App Preview video
- **Android:** Google Play listing with Feature Graphic and Promo Video

---

### User Story 1.2: Initial Download
**As a** new user  
**I want to** quickly download and install the app  
**So that** I can start learning hunting calls immediately

**Acceptance Criteria:**
- [ ] App download size is optimized (<150MB initial download)
- [ ] Installation completes within 2 minutes on average broadband
- [ ] Required permissions are clearly explained before installation
- [ ] App installs without errors on supported devices
- [ ] Post-install, app icon appears on home screen/app drawer

**Platform Notes:**
- **iOS:** TestFlight available for beta testing
- **Android:** APK size optimized with App Bundle

---

## Epic 2: First Launch Experience

### User Story 2.1: Splash Screen & Initialization
**As a** first-time user  
**I want to** see a professional splash screen while the app loads  
**So that** I understand the app is launching and branded correctly

**Acceptance Criteria:**
- [ ] Splash screen displays Hunt Master Academy logo/branding
- [ ] Loading indicator shows initialization progress
- [ ] Initial load completes within 3 seconds on modern devices
- [ ] No crashes or errors during first launch
- [ ] Background audio engine initializes without user intervention

**Technical Requirements:**
- Audio engine (C++ core) initializes asynchronously
- Master call library metadata loads from local cache
- Network connectivity check for cloud features

---

### User Story 2.2: Permissions Request
**As a** new user  
**I want to** understand why the app needs specific permissions  
**So that** I can make informed decisions about granting access

**Acceptance Criteria:**
- [ ] Microphone permission request includes clear explanation: "Record your hunting calls for real-time analysis and feedback"
- [ ] Storage permission (Android) explained for saving recordings and progress
- [ ] Notification permission (optional) explained for practice reminders
- [ ] Location permission (future) explained for field mode features
- [ ] User can deny permissions and still access limited features
- [ ] Permission requests follow platform best practices (just-in-time)

**Platform Notes:**
- **iOS:** App Tracking Transparency (ATT) prompt if analytics enabled
- **Android:** Runtime permissions with rationale dialogs

---

### User Story 2.3: Welcome Tutorial
**As a** first-time user  
**I want to** see a brief interactive tutorial  
**So that** I understand how to use the core features

**Acceptance Criteria:**
- [ ] Tutorial is optional with "Skip" button clearly visible
- [ ] 3-5 screen walkthrough covering:
  - Step 1: Selecting a master call to practice
  - Step 2: Recording your own attempt
  - Step 3: Viewing similarity score and feedback
  - Step 4: Understanding coaching tips
  - Step 5: Tracking progress over time
- [ ] Tutorial uses actual UI screenshots or interactive demo
- [ ] "Get Started" button leads to main app interface
- [ ] Tutorial can be replayed from settings menu

**Technical Requirements:**
- Tutorial state saved to prevent re-showing on subsequent launches
- Animations are smooth (60fps) and accessible

---

## Epic 3: User Registration & Authentication

### User Story 3.1: Account Creation (Email)
**As a** new user  
**I want to** create an account with my email  
**So that** my progress is saved and synced across devices

**Acceptance Criteria:**
- [ ] Email registration form with fields: email, password, confirm password, display name
- [ ] Real-time validation for:
  - Valid email format
  - Password strength (8+ chars, mix of letters/numbers/symbols)
  - Matching password confirmation
  - Display name (3-20 characters, no profanity)
- [ ] "Sign Up" button disabled until all validations pass
- [ ] Terms of Service and Privacy Policy checkboxes (required)
- [ ] Error messages are clear and actionable
- [ ] Success confirmation with email verification prompt
- [ ] Progress indicator during account creation API call

**Platform Notes:**
- **iOS:** Keyboard adjusts input fields, auto-suggest email from contacts
- **Android:** Material Design form styling, autofill support

**API Integration:**
```
POST /api/v1/auth/register
{
  "email": "hunter@example.com",
  "password": "SecurePass123!",
  "displayName": "ProHunter",
  "platform": "ios|android",
  "deviceId": "unique_device_identifier"
}
```

---

### User Story 3.2: Email Verification
**As a** newly registered user  
**I want to** verify my email address  
**So that** my account is fully activated

**Acceptance Criteria:**
- [ ] Verification email sent within 1 minute of registration
- [ ] Email contains branded template with verification link
- [ ] Verification link opens app (deep link) or web confirmation page
- [ ] User can resend verification email if not received
- [ ] Verified status displayed in app settings
- [ ] Reminder prompt shown if not verified within 24 hours
- [ ] Limited features available before verification (can practice, but no cloud sync)

**Platform Notes:**
- **iOS:** Universal Links for email verification deep linking
- **Android:** App Links for seamless verification flow

---

### User Story 3.3: Social Sign-In (OAuth)
**As a** new user  
**I want to** sign up using my Google/Apple/Facebook account  
**So that** I can quickly get started without creating a new password

**Acceptance Criteria:**
- [ ] "Sign in with Google" button (both platforms)
- [ ] "Sign in with Apple" button (iOS required, Android optional)
- [ ] "Sign in with Facebook" button (optional, both platforms)
- [ ] OAuth flow completes within native browser/sheet
- [ ] User returned to app after successful authentication
- [ ] Display name pre-filled from social profile (editable)
- [ ] Profile picture imported from social account (optional)
- [ ] Email auto-verified for social sign-ins
- [ ] Privacy disclosure: "We only access your name and email"

**Platform Notes:**
- **iOS:** Sign in with Apple is **required** per App Store guidelines
- **Android:** Google Sign-In is primary, Apple Sign-In optional

**API Integration:**
```
POST /api/v1/auth/oauth
{
  "provider": "google|apple|facebook",
  "idToken": "oauth_id_token",
  "platform": "ios|android"
}
```

---

### User Story 3.4: Login (Returning Users)
**As a** returning user  
**I want to** log in with my credentials  
**So that** I can access my saved progress and settings

**Acceptance Criteria:**
- [ ] Login screen with email and password fields
- [ ] "Remember Me" checkbox for persistent login
- [ ] "Forgot Password?" link clearly visible
- [ ] Login button validates inputs before API call
- [ ] Biometric login option (Face ID, Touch ID, fingerprint) after first login
- [ ] Error handling for incorrect credentials
- [ ] Account lockout after 5 failed attempts (security)
- [ ] Smooth transition to main app after successful login

**Platform Notes:**
- **iOS:** Keychain stores credentials securely, Face ID/Touch ID via LocalAuthentication
- **Android:** Credential Manager, Biometric Prompt API for fingerprint

**API Integration:**
```
POST /api/v1/auth/login
{
  "email": "hunter@example.com",
  "password": "SecurePass123!",
  "deviceId": "unique_device_identifier"
}

Response:
{
  "accessToken": "jwt_token",
  "refreshToken": "refresh_token",
  "user": {
    "id": "user_123",
    "email": "hunter@example.com",
    "displayName": "ProHunter",
    "skillLevel": "intermediate",
    "profilePicture": "url"
  }
}
```

---

### User Story 3.5: Password Recovery
**As a** user who forgot my password  
**I want to** reset it securely  
**So that** I can regain access to my account

**Acceptance Criteria:**
- [ ] "Forgot Password?" link on login screen
- [ ] Password reset form requests email address
- [ ] Reset email sent within 1 minute
- [ ] Email contains time-limited reset link (valid 1 hour)
- [ ] Reset link opens app or web page with new password form
- [ ] New password meets strength requirements
- [ ] Successful reset confirmation shown
- [ ] User auto-logged in after successful password reset
- [ ] Security: invalidate all existing sessions on password change

---

## Epic 4: Onboarding & Profile Setup

### User Story 4.1: User Profile Customization
**As a** new user  
**I want to** set up my profile  
**So that** the app can personalize my experience

**Acceptance Criteria:**
- [ ] Profile setup wizard after first login (optional skip)
- [ ] Fields to customize:
  - Profile picture (upload from gallery or camera)
  - Display name (editable)
  - Hunting experience level: Beginner | Intermediate | Advanced | Expert
  - Primary hunting interests: Deer, Elk, Turkey, Waterfowl, Predators, Small Game (multi-select)
  - Geographic region: Northeast, Southeast, Midwest, West, etc.
  - Preferred practice time: Morning, Afternoon, Evening
- [ ] "Save Profile" button with validation
- [ ] Profile preview before saving
- [ ] Changes saved to cloud and synced across devices

**Adaptive Features:**
- App recommends starter calls based on experience level
- Master call library filtered by hunting interests
- Practice reminders scheduled at preferred times

---

### User Story 4.2: Initial Skill Assessment (Optional)
**As a** new user  
**I want to** take a quick skill assessment  
**So that** the app can provide appropriate challenges

**Acceptance Criteria:**
- [ ] Optional assessment offered during onboarding
- [ ] Assessment includes 3-5 sample calls (easy to moderate)
- [ ] User records attempts at simple deer grunt, turkey yelp, elk bugle
- [ ] Real-time similarity scoring with instant feedback
- [ ] Assessment takes 5-10 minutes to complete
- [ ] Skill level assigned: Novice (0-30%), Beginner (30-50%), Intermediate (50-70%), Advanced (70-85%), Expert (85%+)
- [ ] Results displayed with encouragement and next steps
- [ ] Skill level can be manually adjusted later in settings
- [ ] Assessment can be retaken monthly to track improvement

**Technical Requirements:**
- Use simplified scoring for assessment (less strict than normal practice)
- Save assessment results to user profile
- Display baseline metrics for progress comparison

---

### User Story 4.3: Notification Preferences
**As a** new user  
**I want to** configure notification settings  
**So that** I receive helpful reminders without being annoyed

**Acceptance Criteria:**
- [ ] Notification preferences screen during onboarding (can skip)
- [ ] Toggle options for:
  - Daily practice reminders (on/off, time selection)
  - Weekly progress reports (on/off)
  - Achievement unlocks (on/off)
  - New master calls added (on/off)
  - Community challenges (on/off, future)
  - App updates and news (on/off)
- [ ] Quiet hours setting: Don't send notifications during specified hours
- [ ] Sample notification shown for each type
- [ ] "Test Notification" button to preview
- [ ] Settings saved and synced to backend
- [ ] Can be changed anytime in Settings menu

**Platform Notes:**
- **iOS:** Notification settings link to iOS Settings app
- **Android:** In-app notification channel management

---

## Epic 5: Master Call Library & Selection

### User Story 5.1: Browse Master Calls
**As a** user  
**I want to** browse available master calls  
**So that** I can choose what to practice

**Acceptance Criteria:**
- [ ] Master call library screen with categorized list:
  - By Species: Whitetail Deer, Elk, Turkey, Waterfowl, Predators
  - By Difficulty: Beginner, Intermediate, Advanced
  - By Call Type: Grunt, Bugle, Yelp, Quack, Howl, etc.
  - Favorites (user-saved)
  - Recently Practiced
  - Recommended for You (based on skill level)
- [ ] Each call displays:
  - Call name and species
  - Difficulty badge with color coding
  - Duration (e.g., "2.5 seconds")
  - Thumbnail waveform preview
  - "Play" button for preview
  - "Favorite" heart icon
  - User's best score (if practiced before)
- [ ] Search bar filters calls by name, species, type
- [ ] Pull-to-refresh updates library from cloud
- [ ] Loading state for network requests
- [ ] Offline mode: access previously downloaded calls

**Platform Notes:**
- **iOS:** Native list with smooth scrolling, 3D Touch preview
- **Android:** RecyclerView with efficient view holder pattern

**API Integration:**
```
GET /api/v1/gamecalls/masters?species=deer&difficulty=intermediate&limit=20

Response:
{
  "masters": [
    {
      "id": "whitetail-grunt-01",
      "name": "Adult Buck Grunt",
      "species": "whitetail-deer",
      "callType": "grunt",
      "difficulty": "intermediate",
      "durationSec": 2.5,
      "waveformUrl": "url_to_preview",
      "audioUrl": "url_to_full_audio",
      "description": "Classic buck grunt for rutting season",
      "tags": ["rut", "dominance", "contact"]
    }
  ]
}
```

---

### User Story 5.2: Preview Master Call
**As a** user  
**I want to** listen to a master call before practicing  
**So that** I know what the target sound should be

**Acceptance Criteria:**
- [ ] Tap on call card expands details view
- [ ] Expanded view shows:
  - Full waveform visualization (zoomable)
  - Call description and usage context
  - Vocal technique tips
  - Ideal scenarios for use in field
  - Related calls (e.g., "Also try: Doe Bleat")
- [ ] Large "Play" button with clear play/pause states
- [ ] Playback controls: play, pause, replay, adjust volume
- [ ] Playback speed control: 0.5x, 0.75x, 1.0x, 1.25x (for learning)
- [ ] Waveform highlights during playback (visual sync)
- [ ] "Practice This Call" button to start session
- [ ] "Download for Offline" button (if not cached)
- [ ] Background playback continues if user leaves details view

**Technical Requirements:**
- Audio streaming from CDN with local caching
- Waveform rendered from pre-computed peak data
- Low latency playback using platform audio APIs

---

### User Story 5.3: Favorite and Organize Calls
**As a** user  
**I want to** save favorite calls  
**So that** I can quickly access calls I practice frequently

**Acceptance Criteria:**
- [ ] Heart icon on each call card to favorite/unfavorite
- [ ] Visual feedback on tap (animation, color change)
- [ ] Favorites tab in library shows all saved calls
- [ ] Favorites synced to cloud (accessible across devices)
- [ ] Create custom playlists (future enhancement):
  - "Morning Routine"
  - "Pre-Season Practice"
  - "Advanced Challenge"
- [ ] Reorder favorites by drag-and-drop
- [ ] Remove from favorites with confirmation
- [ ] Badge showing number of favorites in tab bar

**API Integration:**
```
POST /api/v1/users/favorites
{
  "masterId": "whitetail-grunt-01"
}

DELETE /api/v1/users/favorites/whitetail-grunt-01
```

---

## Epic 6: Practice Session & Recording

### User Story 6.1: Start Practice Session
**As a** user  
**I want to** start a practice session with a selected master call  
**So that** I can record my attempt and receive feedback

**Acceptance Criteria:**
- [ ] "Practice This Call" button from call details
- [ ] Session setup screen shows:
  - Master call name and species
  - Waveform preview reminder
  - Estimated session duration
  - "Listen Again" option
  - Microphone check indicator (green = good, yellow = low, red = too loud)
- [ ] Countdown before recording: "Ready in 3... 2... 1..."
- [ ] Visual recording indicator (red dot, pulsing animation)
- [ ] Real-time audio level meter during recording
- [ ] Waveform builds in real-time as user records
- [ ] "Stop" button to end recording early
- [ ] Auto-stop after 30 seconds or when silence detected
- [ ] Graceful handling of microphone errors (permission denied, hardware issue)

**Technical Requirements:**
- Audio recording at 44.1kHz sample rate (engine requirement)
- Real-time streaming to C++ audio engine for analysis
- VAD (Voice Activity Detection) to auto-detect call start/end
- Low latency feedback (<100ms from recording to display)

**Platform Notes:**
- **iOS:** AVAudioEngine for low-latency capture
- **Android:** AudioRecord API with buffer management

---

### User Story 6.2: Real-Time Similarity Feedback
**As a** user recording a call  
**I want to** see real-time similarity feedback  
**So that** I can adjust my technique during the attempt

**Acceptance Criteria:**
- [ ] Similarity score gauge updates in real-time (0-100%)
- [ ] Gauge color changes: Red (<30%), Yellow (30-70%), Green (70%+)
- [ ] Smooth animation as score updates (no jittery jumps)
- [ ] Peak score indicator shows best moment during recording
- [ ] Mini coaching hints displayed if score drops:
  - "Pitch too high" / "Pitch too low"
  - "More consistent rhythm"
  - "Louder volume needed"
- [ ] Waveform overlay: user's attempt vs. master call (aligned)
- [ ] Haptic/vibration feedback for significant score changes (optional)
- [ ] Mute coaching hints option for distraction-free practice

**Technical Requirements:**
- Integration with UnifiedAudioEngine real-time scorer
- Update UI at 10-15 FPS for smooth feedback without overwhelming
- Buffer analysis results to avoid UI lag

**API Integration:**
```
POST /api/v1/gamecalls/sessions/:sessionId/analyze
{
  "audioChunk": "base64_encoded_pcm_data",
  "chunkIndex": 0,
  "timestamp": "2025-11-08T10:30:01.500Z"
}

Response:
{
  "similarity": {
    "score": 0.72,
    "confidence": 0.85,
    "reliable": true
  },
  "pitch": { "frequency": 245.5, "confidence": 0.78 },
  "harmonic": { "richness": 0.65, "clarity": 0.72 },
  "cadence": { "tempo": 120, "consistency": 0.82 }
}
```

---

### User Story 6.3: Session Completion & Results
**As a** user who finished recording  
**I want to** see detailed results and feedback  
**So that** I understand how well I performed

**Acceptance Criteria:**
- [ ] Results screen displays:
  - Overall similarity score (large, prominent display)
  - Score breakdown:
    - Pitch accuracy: Grade (A-F) and percentage
    - Harmonic quality: Grade and percentage
    - Cadence rhythm: Grade and percentage
    - Volume consistency: Grade and percentage
  - Comparison waveform: Master (blue) vs. User (green) overlaid
  - Time-series graph showing similarity over duration
  - Best segment highlighted (if using finalization)
- [ ] Celebratory animation for high scores (confetti, badge unlock)
- [ ] Coaching feedback section with actionable tips:
  - "Great job! Pitch was spot-on."
  - "Try to maintain more consistent rhythm in the middle section."
  - "Volume could be slightly louder to match intensity."
- [ ] "Listen to Recording" button to replay user's attempt
- [ ] "Listen to Master" button for comparison
- [ ] "Try Again" button to retry immediately
- [ ] "Share Results" button (social media, leaderboard)
- [ ] "Save Recording" option (optional, for personal review)
- [ ] Progress bar showing improvement from last attempt

**Technical Requirements:**
- Call finalizeSessionAnalysis API for refined metrics
- Cache results locally for offline viewing
- Compress audio if saving to device storage

**API Integration:**
```
POST /api/v1/gamecalls/sessions/:sessionId/finalize

Response:
{
  "sessionId": "sess_abc123",
  "status": "finalized",
  "finalMetrics": {
    "overallSimilarity": 0.78,
    "pitchGrade": "B",
    "harmonicGrade": "B+",
    "cadenceGrade": "A-",
    "loudnessDeviation": 0.12,
    "segment": { "startMs": 500, "durationMs": 1200 }
  },
  "coaching": [
    {
      "category": "pitch",
      "severity": "info",
      "message": "Good pitch control. Minor stability improvement at onset."
    }
  ]
}
```

---

### User Story 6.4: Practice History & Progress Tracking
**As a** user  
**I want to** view my practice history  
**So that** I can track improvement over time

**Acceptance Criteria:**
- [ ] "History" tab shows chronological list of all practice sessions
- [ ] Each session entry displays:
  - Date and time
  - Master call practiced
  - Overall similarity score
  - Score trend indicator: ↑ improved, ↓ declined, → similar
  - Thumbnail waveform
  - Duration
- [ ] Filter history by:
  - Date range (last week, month, 3 months, all time)
  - Species
  - Call type
  - Score range (only show scores >70%)
- [ ] Tap session to view detailed results
- [ ] "Delete Session" option with confirmation (swipe to delete)
- [ ] Progress graph showing score trends over time
- [ ] Statistics dashboard:
  - Total practice time
  - Average score by species
  - Improvement rate (% per week)
  - Current streak (days practiced consecutively)
  - Best call (highest average score)
- [ ] Export history as CSV (for advanced users)

**Platform Notes:**
- **iOS:** SwiftUI Charts for progress graphs
- **Android:** MPAndroidChart library for visualizations

---

## Epic 7: Group Creation & Social Features

### User Story 7.1: Create Hunting Group
**As a** user  
**I want to** create a hunting group with friends  
**So that** we can practice together and share progress

**Acceptance Criteria:**
- [ ] "Groups" tab in main navigation
- [ ] "Create New Group" button with form:
  - Group name (3-50 characters, unique)
  - Description (optional, 200 char max)
  - Privacy: Public (anyone can join) | Private (invite-only)
  - Group avatar (upload or choose from presets)
  - Tags: #DeerHunting #TurkeyHunting #BeginnerFriendly (optional)
- [ ] Group created after validation
- [ ] Creator automatically assigned as admin
- [ ] Confirmation message with share link
- [ ] Group appears in user's "My Groups" list

**API Integration:**
```
POST /api/v1/groups
{
  "name": "Midwest Deer Hunters",
  "description": "Practice group for whitetail enthusiasts",
  "privacy": "private",
  "avatarUrl": "url_to_image",
  "tags": ["deer", "midwest", "beginner"]
}

Response:
{
  "groupId": "grp_xyz789",
  "name": "Midwest Deer Hunters",
  "shareCode": "JOIN-MWDH-2025",
  "inviteLink": "https://hma.app/groups/grp_xyz789/join",
  "createdAt": "2025-11-08T10:30:00Z"
}
```

---

### User Story 7.2: Invite Members to Group
**As a** group admin  
**I want to** invite others to join my group  
**So that** we can practice and compete together

**Acceptance Criteria:**
- [ ] "Invite Members" button in group settings
- [ ] Invitation methods:
  - Share invite link (universal link)
  - Share join code (e.g., "JOIN-MWDH-2025")
  - Invite by email (enter email addresses)
  - Share via SMS or social media
  - Generate QR code for in-person invites
- [ ] Invite link opens app and displays group preview
- [ ] Non-users redirected to app download page first
- [ ] Join request approval for private groups (admin review)
- [ ] Auto-join for public groups
- [ ] Push notification when someone joins
- [ ] Invite tracking: Pending, Accepted, Declined
- [ ] Invite expiration: 7 days for security

**Platform Notes:**
- **iOS:** Universal Links for seamless app opening
- **Android:** App Links with fallback to web preview

---

### User Story 7.3: Join Existing Group
**As a** user  
**I want to** join an existing hunting group  
**So that** I can participate in group activities

**Acceptance Criteria:**
- [ ] "Join Group" button in Groups tab
- [ ] Search groups by:
  - Group name
  - Tags (#DeerHunting, etc.)
  - Location/region
- [ ] Browse public groups with filters:
  - Most active
  - Newest
  - Recommended (based on user interests)
- [ ] Group preview shows:
  - Group name and avatar
  - Description
  - Member count
  - Recent activity
  - Privacy status
- [ ] "Join" button for public groups (instant)
- [ ] "Request to Join" for private groups (awaits approval)
- [ ] Enter join code manually if have one
- [ ] Scan QR code to join instantly
- [ ] Confirmation message on successful join
- [ ] Group appears in "My Groups" list

**API Integration:**
```
POST /api/v1/groups/:groupId/join
{
  "userId": "user_123",
  "joinCode": "JOIN-MWDH-2025"  // optional
}

Response:
{
  "status": "joined" | "pending_approval",
  "groupId": "grp_xyz789",
  "message": "Welcome to Midwest Deer Hunters!"
}
```

---

### User Story 7.4: Group Activity Feed
**As a** group member  
**I want to** see what my group members are doing  
**So that** I can stay engaged and motivated

**Acceptance Criteria:**
- [ ] Group detail view shows activity feed:
  - Member joined: "John Doe joined the group"
  - Practice session: "Jane Smith scored 85% on Buck Grunt"
  - Achievement: "Mike Johnson unlocked 'Perfect Call' badge"
  - Challenge completed: "Sarah completed Weekly Challenge"
  - New high score: "Tom set new group record: 92%"
- [ ] Activities displayed chronologically (newest first)
- [ ] Pull-to-refresh updates feed
- [ ] Like/reaction buttons on activities (👍 🔥 🎯)
- [ ] Comment on activities (optional, future)
- [ ] Filter feed: All Activities | Achievements | High Scores | Challenges
- [ ] Push notification for significant events:
  - Group member beats your high score
  - Someone mentions you in a comment
  - New challenge posted
- [ ] Activity details on tap (view full session results)

**Technical Requirements:**
- Real-time updates via WebSocket or long polling
- Activity cache for offline viewing
- Pagination for long feeds (infinite scroll)

---

### User Story 7.5: Group Leaderboards
**As a** group member  
**I want to** see how I rank compared to others  
**So that** I can compete and improve

**Acceptance Criteria:**
- [ ] Leaderboard tab in group view
- [ ] Leaderboard types:
  - Overall Average Score (all calls)
  - Top Scores by Species (Deer, Turkey, Elk, etc.)
  - Most Improved This Week
  - Practice Streak (consecutive days)
  - Total Practice Time
- [ ] Leaderboard displays:
  - Rank (1st, 2nd, 3rd with medals, 4+)
  - User avatar and display name
  - Relevant metric (score, time, streak)
  - Trend indicator (rank change from last period)
- [ ] Current user highlighted (different color)
- [ ] Time range selector: This Week | This Month | All Time
- [ ] Tap user to view their profile (if permissions allow)
- [ ] "Challenge" button to send friendly competition invite
- [ ] Leaderboard refreshes daily at midnight
- [ ] Top 3 receive special badges (Gold, Silver, Bronze)

**Platform Notes:**
- Leaderboards comply with platform guidelines (no gambling/betting)
- Privacy settings respected (users can opt-out of leaderboards)

---

### User Story 7.6: Group Challenges
**As a** group admin  
**I want to** create challenges for group members  
**So that** we can have structured practice goals

**Acceptance Criteria:**
- [ ] "Create Challenge" button (admin only)
- [ ] Challenge form:
  - Challenge name (e.g., "Perfect Buck Grunt Week")
  - Description and rules
  - Target call(s) to practice
  - Goal: Achieve X score | Complete Y sessions | Practice Z minutes
  - Duration: Start date and end date
  - Reward: Badge, title, or custom prize description
- [ ] Challenge posted to group feed
- [ ] Push notification sent to all members
- [ ] Challenge card shows:
  - Challenge details
  - Time remaining (countdown)
  - Participation count
  - Progress tracker (personal and group)
  - Leaderboard for challenge participants
- [ ] "Accept Challenge" button to opt-in
- [ ] Automatic progress tracking as users practice
- [ ] Completion celebration when goal met (confetti, badge unlock)
- [ ] Results summary at challenge end with winners announced

**API Integration:**
```
POST /api/v1/groups/:groupId/challenges
{
  "name": "Perfect Buck Grunt Week",
  "description": "Score 80%+ on buck grunt 5 times",
  "masterId": "whitetail-grunt-01",
  "goalType": "score_threshold",
  "goalValue": 80,
  "requiredSessions": 5,
  "startDate": "2025-11-10T00:00:00Z",
  "endDate": "2025-11-17T23:59:59Z",
  "reward": "Buck Master Badge"
}
```

---

## Epic 8: Gamification & Achievements

### User Story 8.1: Achievement System
**As a** user  
**I want to** earn achievements for milestones  
**So that** I feel rewarded for my progress

**Acceptance Criteria:**
- [ ] Achievement categories:
  - **Accuracy:** "First Perfect Call" (100% score), "Sharpshooter" (90%+ on 10 calls)
  - **Consistency:** "7-Day Streak", "30-Day Streak", "100-Day Streak"
  - **Species Mastery:** "Deer Expert" (50 deer calls), "Turkey Master", "Elk Champion"
  - **Dedication:** "100 Sessions", "1000 Sessions", "10,000 Minutes Practiced"
  - **Community:** "Group Founder", "Challenge Champion", "Helpful Coach" (10 tips shared)
  - **Exploration:** "Tried All Calls", "Every Species", "Beginner to Expert"
- [ ] Achievement unlock animation:
  - Full-screen badge reveal with confetti
  - Sound effect (celebratory chime)
  - Share prompt to social media/group
- [ ] Achievements displayed in user profile
- [ ] Progress bars for incomplete achievements
- [ ] Locked achievements shown in silhouette with requirements
- [ ] Achievement rarity: Common, Rare, Epic, Legendary (color-coded)
- [ ] Total achievement points displayed (gamification currency)

**Technical Requirements:**
- Achievement unlock conditions checked server-side
- Badge images pre-loaded to avoid display lag
- Achievement state synced across devices

---

### User Story 8.2: Daily & Weekly Challenges
**As a** user  
**I want to** complete daily challenges  
**So that** I have structured practice goals

**Acceptance Criteria:**
- [ ] "Challenges" tab in main navigation
- [ ] Daily Challenge (resets every 24 hours):
  - Randomly selected easy call
  - Goal: Score 70%+ once
  - Reward: 10 XP, progress toward streak
- [ ] Weekly Challenge (resets every Monday):
  - Moderate difficulty, specific species focus
  - Goal: Score 80%+ on 3 different calls
  - Reward: 100 XP, exclusive badge
- [ ] Challenge preview card shows:
  - Challenge name and description
  - Time remaining (countdown timer)
  - Reward preview
  - Participation stats (X users completed)
- [ ] Progress tracking for multi-session challenges
- [ ] Completion notification with reward claimed
- [ ] Missed challenges shown in history (for reference)
- [ ] Challenge recommendations based on skill level

**Platform Notes:**
- Challenges refresh automatically with push notification reminder
- Offline attempts queued and validated when online

---

### User Story 8.3: Skill Level Progression
**As a** user  
**I want to** level up as I improve  
**So that** I can see my long-term progression

**Acceptance Criteria:**
- [ ] User level system: Level 1 (Novice) → Level 50 (Master Hunter)
- [ ] XP (experience points) earned from:
  - Completing practice sessions: 5-20 XP (based on score)
  - Achieving high scores (80%+): Bonus 10 XP
  - Daily login: 5 XP
  - Completing challenges: 10-100 XP
  - Unlocking achievements: 25-500 XP (rarity-based)
  - Helping group members: 5 XP per interaction
- [ ] Level-up animation and notification
- [ ] Level displayed on profile with progress bar to next level
- [ ] Level unlocks features:
  - Level 5: Access to intermediate calls
  - Level 10: Custom profile themes
  - Level 15: Advanced coaching insights
  - Level 20: Create group challenges
  - Level 25: Access to expert calls
- [ ] Level badges (Bronze, Silver, Gold, Platinum, Diamond tiers)
- [ ] Leaderboard sorted by level (global, regional, group)

**Technical Requirements:**
- XP formula: `XP_required = 100 * (level ^ 1.5)`
- Level-up notifications via push when app not active
- Level progress synced to prevent loss on device change

---

### User Story 8.4: Streaks & Consistency Rewards
**As a** user  
**I want to** maintain practice streaks  
**So that** I stay motivated to practice regularly

**Acceptance Criteria:**
- [ ] Streak counter displayed prominently on home screen
- [ ] Current streak: "🔥 7 Day Streak!"
- [ ] Longest streak: "Personal Best: 45 Days"
- [ ] Streak maintained by completing at least 1 practice session per day
- [ ] Streak freezes available: 3 per month (skip a day without breaking streak)
- [ ] Streak reminder notification: "Don't break your streak! Practice today."
- [ ] Milestone streak rewards:
  - 7 days: "Week Warrior" badge + 50 XP
  - 30 days: "Monthly Master" badge + 200 XP
  - 100 days: "Century Club" badge + 1000 XP
- [ ] Streak loss notification (motivational, not punishing):
  - "Streak ended at 23 days. Start a new one today!"
- [ ] Streak leaderboard (global and group)
- [ ] "Streak Saver" power-up (in-app purchase or reward)

**Platform Notes:**
- Streak calculated in user's local timezone
- Grace period: 6-hour buffer after midnight before streak breaks

---

## Epic 9: Advanced Features & Settings

### User Story 9.1: Audio Settings & Calibration
**As a** user  
**I want to** adjust audio settings  
**So that** the app works optimally with my device

**Acceptance Criteria:**
- [ ] Audio settings menu in Settings:
  - Input device selection (if multiple mics available)
  - Input gain/sensitivity slider (auto or manual)
  - Noise reduction: On/Off (uses environment profiler)
  - Echo cancellation: On/Off
  - Sample rate display (informational)
  - Latency compensation (auto-detected)
- [ ] Microphone calibration wizard:
  - "Test Your Mic" button
  - Record 5-second sample at different volumes
  - App suggests optimal input gain
  - Visual meter shows input level (green zone target)
  - "Recalibrate" option if settings change
- [ ] Headphone/speaker toggle:
  - "Use headphones for better feedback" prompt
  - Audio output selection (speaker, headphones, Bluetooth)
- [ ] Playback volume control (separate from system volume)
- [ ] Background audio handling:
  - Continue recording if phone call received?
  - Pause recording if app backgrounded?

**Technical Requirements:**
- Audio configuration stored per device (device-specific settings)
- Integration with UnifiedAudioEngine audio level processor
- Real-time input monitoring without recording

---

### User Story 9.2: Offline Mode
**As a** user  
**I want to** practice without an internet connection  
**So that** I can use the app in remote hunting locations

**Acceptance Criteria:**
- [ ] Offline indicator displayed when no network (airplane icon)
- [ ] Cached content available offline:
  - Downloaded master calls (up to 50 MB configurable cache)
  - Recent practice history (last 30 sessions)
  - User profile and settings
  - Achievements and progress (local state)
- [ ] Practice sessions work fully offline:
  - Record attempts
  - Real-time analysis (local C++ engine)
  - View results and coaching
- [ ] "Download for Offline" button on master calls
- [ ] Offline queue for sync when online:
  - Practice results uploaded automatically
  - Achievements unlocked retroactively
  - Leaderboards updated
- [ ] Sync indicator: "Syncing 3 sessions..." with progress
- [ ] Conflict resolution if same call practiced on multiple devices
- [ ] Storage management: Clear cache option, auto-cleanup old sessions

**Technical Requirements:**
- SQLite local database for offline storage
- Background sync task when network restored
- Delta sync to minimize data usage

---

### User Story 9.3: Accessibility Features
**As a** user with accessibility needs  
**I want to** customize the app for my requirements  
**So that** I can use it comfortably

**Acceptance Criteria:**
- [ ] Accessibility settings menu:
  - Text size: Small, Medium, Large, Extra Large
  - High contrast mode (increased color difference)
  - Color blind modes: Deuteranopia, Protanopia, Tritanopia
  - Reduce motion (disable animations)
  - Screen reader compatibility (VoiceOver, TalkBack)
  - Haptic feedback intensity: Off, Low, Medium, High
- [ ] Visual indicators for audio events:
  - Visual recording indicator (for hearing impaired)
  - Visual metronome for cadence practice
  - Closed captions for tutorial videos
- [ ] Alternative input methods:
  - Large tap targets (minimum 44x44 points)
  - Voice commands: "Start practice", "Play master call"
  - External switch support (iOS)
- [ ] Audio descriptions for graphs and visualizations
- [ ] Compliant with WCAG 2.1 AA standards
- [ ] "Report Accessibility Issue" button

**Platform Notes:**
- **iOS:** Full VoiceOver support, Dynamic Type
- **Android:** TalkBack support, Font scaling

---

### User Story 9.4: Data & Privacy Controls
**As a** user  
**I want to** control my data and privacy  
**So that** I feel secure using the app

**Acceptance Criteria:**
- [ ] Privacy settings menu:
  - Profile visibility: Public, Friends Only, Private
  - Show real name vs. display name only
  - Hide from leaderboards (opt-out)
  - Share progress in groups: On/Off
  - Activity feed visibility: All, Friends, None
  - Email notifications: On/Off
- [ ] Data management:
  - "Download My Data" (GDPR compliance): ZIP file with all user data
  - "Delete Practice Session" (individual)
  - "Clear All History" (with confirmation)
  - "Delete Account" (permanent, requires password confirmation)
- [ ] Privacy policy and terms of service links
- [ ] Analytics opt-out: "Help improve HMA by sharing usage data"
- [ ] Third-party data sharing transparency
- [ ] Cookie/tracking consent (web version)
- [ ] Parental controls (age verification for social features)

**API Integration:**
```
GET /api/v1/users/:userId/data
Authorization: Bearer {token}

Response: ZIP file with:
- profile.json
- sessions.json
- achievements.json
- groups.json
```

---

### User Story 9.5: In-App Support & Help
**As a** user  
**I want to** easily get help when needed  
**So that** I can resolve issues quickly

**Acceptance Criteria:**
- [ ] Help & Support menu:
  - FAQ section (common questions)
  - Video tutorials (embedded or YouTube links)
  - "Contact Support" form (email support)
  - Live chat (future, if staffed)
  - Community forum link
- [ ] Contextual help:
  - "?" icon on complex screens
  - Tooltips on first use (dismissible)
  - "How does this work?" button
- [ ] Troubleshooting guides:
  - "Microphone not working"
  - "Low similarity scores"
  - "App crashes"
  - "Sync issues"
- [ ] Bug report tool:
  - Screenshot capture
  - Auto-attach device info and logs
  - Priority: Low, Medium, High, Critical
  - Confirmation email sent
- [ ] Feature request form
- [ ] App version and device info displayed (for support)

**Platform Notes:**
- **iOS:** In-app feedback assistant integration
- **Android:** Shake to send feedback gesture

---

## Epic 10: In-App Purchases & Monetization (Optional)

### User Story 10.1: Free vs. Premium Features
**As a** free user  
**I want to** understand what features are available  
**So that** I can decide if upgrading is worthwhile

**Acceptance Criteria:**
- [ ] Free tier includes:
  - 3 practice sessions per day
  - Access to 10 beginner master calls
  - Basic similarity scoring
  - Limited group features (join only)
  - Ads displayed (non-intrusive banner ads)
- [ ] Premium tier ($4.99/month or $49.99/year):
  - Unlimited practice sessions
  - Access to 200+ master calls (all difficulties)
  - Advanced coaching insights (pitch, harmonic, cadence)
  - Create unlimited groups
  - Ad-free experience
  - Exclusive premium challenges
  - Priority customer support
  - Offline mode with unlimited downloads
- [ ] "Upgrade to Premium" prompts:
  - Banner at top of library (dismissible)
  - Interstitial after 3rd session of the day
  - Feature teaser: "Unlock advanced calls with Premium"
- [ ] Free trial: 7 days (first-time users only)
- [ ] Pricing displayed clearly (with local currency)
- [ ] "Restore Purchases" button for existing subscribers

**Platform Notes:**
- **iOS:** In-App Purchase via StoreKit, subscription auto-renewal
- **Android:** In-App Billing, Google Play subscription

---

### User Story 10.2: Subscription Management
**As a** premium subscriber  
**I want to** manage my subscription  
**So that** I can cancel or change plans easily

**Acceptance Criteria:**
- [ ] Subscription status displayed in Settings:
  - "Premium Active" with expiration date
  - Plan type: Monthly or Annual
  - Next billing date
  - Payment method (last 4 digits)
- [ ] "Manage Subscription" button:
  - iOS: Opens App Store subscriptions
  - Android: Opens Google Play subscriptions
- [ ] Cancel subscription:
  - "Cancel Subscription" button with confirmation
  - Warning: "Premium features expire on [date]"
  - Exit survey (optional): Why are you canceling?
- [ ] Reactivate subscription if lapsed
- [ ] Upgrade from monthly to annual (pro-rated credit)
- [ ] Subscription renewal notifications
- [ ] Payment failure handling (retry, update payment)

**Platform Notes:**
- Platform manages billing; app queries subscription status
- Server-side receipt validation for security

---

## Epic 11: Field Mode Features (Future)

### User Story 11.1: GPS-Based Call Recommendations
**As a** hunter in the field  
**I want to** get call recommendations based on my location  
**So that** I use regionally appropriate calls

**Acceptance Criteria:**
- [ ] "Field Mode" toggle in app
- [ ] GPS detects user's location (with permission)
- [ ] Call library filtered by:
  - Regional species (e.g., show Mule Deer in Western US)
  - Seasonal calls (rut calls in fall, mating calls in spring)
  - Hunting regulations (no calls for closed seasons)
- [ ] Quick-access favorites optimized for field use
- [ ] Compass integration for wind direction awareness
- [ ] Dark mode for low-light field conditions
- [ ] Large buttons for gloved hands
- [ ] Simplified UI (minimal distractions)

---

### User Story 11.2: Environmental Conditions Tracker
**As a** hunter in the field  
**I want to** log environmental conditions with my practice  
**So that** I can correlate conditions with successful calls

**Acceptance Criteria:**
- [ ] Log conditions before practice:
  - Weather: Clear, Overcast, Rain, Snow, Fog
  - Temperature (auto-detected or manual)
  - Wind speed and direction
  - Time of day: Dawn, Morning, Midday, Afternoon, Dusk, Night
  - Moon phase (auto-calculated)
- [ ] Conditions saved with practice session
- [ ] Analytics: "You score highest on clear, calm mornings"
- [ ] Share conditions with group (optional)

---

## Epic 12: Platform-Specific Features

### User Story 12.1: iOS-Specific Features
**As an** iOS user  
**I want to** use iOS-native features  
**So that** the app feels integrated with my device

**Acceptance Criteria:**
- [ ] Widgets (Home Screen and Lock Screen):
  - Practice streak widget
  - Daily challenge widget
  - Quick start practice widget
- [ ] Live Activities (iOS 16+): Real-time practice session progress on lock screen
- [ ] App Clips: QR code at hunting store launches mini-version for quick trial
- [ ] Siri Shortcuts:
  - "Hey Siri, start deer grunt practice"
  - "Hey Siri, show my practice stats"
- [ ] Handoff support: Start practice on iPhone, continue on iPad
- [ ] Apple Watch companion app:
  - View current streak
  - Start practice session remotely (phone does recording)
  - Quick view of last score
- [ ] Focus Mode integration: "Hunting Mode" profile
- [ ] AirDrop sharing for group invites

---

### User Story 12.2: Android-Specific Features
**As an** Android user  
**I want to** use Android-native features  
**So that** the app leverages my device capabilities

**Acceptance Criteria:**
- [ ] Widgets (Home Screen):
  - Practice streak widget (resizable)
  - Daily challenge widget
  - Master call quick launch widget
- [ ] Quick Settings Tile: Toggle recording from notification shade
- [ ] Material You dynamic theming: App colors adapt to wallpaper
- [ ] Google Assistant Actions:
  - "Hey Google, open Hunt Master Academy"
  - "Hey Google, start turkey call practice"
- [ ] Wear OS companion app:
  - View streak and stats
  - Remote start practice session
  - Quick score glance
- [ ] Android Auto support (future): Hands-free practice for truck/vehicle
- [ ] Nearby Share for group invites
- [ ] Work profile support for dual-use devices

---

## Technical Architecture Summary

### Mobile App Stack Recommendations

#### iOS
- **Language:** Swift 5.9+ or React Native 0.72+
- **UI Framework:** SwiftUI (native) or React Native
- **Audio:** AVFoundation (low-latency capture)
- **Networking:** URLSession (native) or Axios (React Native)
- **Storage:** Core Data or Realm (local), Firebase/Supabase (cloud)
- **Authentication:** Firebase Auth or custom JWT
- **Analytics:** Firebase Analytics
- **Crash Reporting:** Firebase Crashlytics
- **Push Notifications:** Firebase Cloud Messaging
- **CI/CD:** Xcode Cloud or GitHub Actions

#### Android
- **Language:** Kotlin 1.9+ or React Native 0.72+
- **UI Framework:** Jetpack Compose (native) or React Native
- **Audio:** AudioRecord API (low-latency capture)
- **Networking:** Retrofit or Axios (React Native)
- **Storage:** Room (local), Firebase/Supabase (cloud)
- **Authentication:** Firebase Auth or custom JWT
- **Analytics:** Firebase Analytics
- **Crash Reporting:** Firebase Crashlytics
- **Push Notifications:** Firebase Cloud Messaging
- **CI/CD:** Google Play Console or GitHub Actions

---

## API Endpoints Summary

### Authentication
- `POST /api/v1/auth/register` - Create account
- `POST /api/v1/auth/login` - Login
- `POST /api/v1/auth/oauth` - Social login
- `POST /api/v1/auth/refresh` - Refresh token
- `POST /api/v1/auth/logout` - Logout
- `POST /api/v1/auth/password-reset` - Request password reset

### User Profile
- `GET /api/v1/users/:id` - Get user profile
- `PUT /api/v1/users/:id` - Update profile
- `GET /api/v1/users/:id/stats` - Get user statistics
- `GET /api/v1/users/:id/achievements` - Get achievements
- `POST /api/v1/users/:id/favorites` - Add favorite call
- `DELETE /api/v1/users/:id/favorites/:masterId` - Remove favorite

### GameCalls Sessions
- `POST /api/v1/gamecalls/sessions` - Create practice session
- `POST /api/v1/gamecalls/sessions/:id/analyze` - Stream audio chunk
- `POST /api/v1/gamecalls/sessions/:id/finalize` - Finalize session
- `GET /api/v1/gamecalls/sessions/:id/results` - Get session results
- `GET /api/v1/gamecalls/sessions/:id/overlay` - Get waveform overlay data

### Master Calls
- `GET /api/v1/gamecalls/masters` - List master calls
- `GET /api/v1/gamecalls/masters/:id` - Get master call details
- `GET /api/v1/gamecalls/masters/:id/audio` - Download audio file

### Groups
- `POST /api/v1/groups` - Create group
- `GET /api/v1/groups/:id` - Get group details
- `POST /api/v1/groups/:id/join` - Join group
- `DELETE /api/v1/groups/:id/leave` - Leave group
- `GET /api/v1/groups/:id/members` - List members
- `GET /api/v1/groups/:id/leaderboard` - Get group leaderboard
- `POST /api/v1/groups/:id/challenges` - Create challenge
- `GET /api/v1/groups/:id/activity` - Get activity feed

### Challenges
- `GET /api/v1/challenges/daily` - Get daily challenge
- `GET /api/v1/challenges/weekly` - Get weekly challenge
- `POST /api/v1/challenges/:id/accept` - Accept challenge
- `POST /api/v1/challenges/:id/complete` - Mark challenge complete

---

## Success Metrics & KPIs

### User Engagement
- Daily Active Users (DAU)
- Monthly Active Users (MAU)
- Average session duration: Target 10-15 minutes
- Sessions per user per week: Target 3-5 sessions
- Retention rates:
  - Day 1: 60%+
  - Day 7: 40%+
  - Day 30: 25%+

### Feature Adoption
- % users completing onboarding: Target 80%+
- % users joining at least one group: Target 50%+
- % users completing daily challenge: Target 30%+
- % users reaching 7-day streak: Target 20%+

### Performance
- App crash rate: <0.1%
- API response time: <200ms (p95)
- Audio analysis latency: <100ms
- App load time: <3 seconds

### Monetization (if applicable)
- Free to Premium conversion: Target 5-10%
- Premium subscriber retention: Target 70%+ after 3 months
- Average Revenue Per User (ARPU)

---

## Accessibility & Compliance

### Compliance Requirements
- **GDPR (Europe):** Data export, deletion, consent management
- **COPPA (US):** Age verification, parental consent for users <13
- **CCPA (California):** Do Not Sell My Data option
- **App Store Review Guidelines:** Content policy, in-app purchases, privacy
- **Google Play Policies:** Content policy, data safety, permissions

### Accessibility Standards
- **WCAG 2.1 Level AA** compliance
- Screen reader compatibility (VoiceOver, TalkBack)
- Minimum contrast ratio 4.5:1 for text
- Minimum tap target size 44x44 points
- Keyboard navigation support
- Closed captions for multimedia

---

## Localization & Internationalization (Future)

### Priority Languages
1. English (US, UK, Canada)
2. Spanish (Spain, Latin America)
3. French (Canada)
4. German

### Localization Considerations
- Species names vary by region
- Imperial vs. metric units
- Date/time formats
- Currency for in-app purchases
- Hunting terminology differs culturally

---

## Testing Strategy

### Test Phases
1. **Alpha Testing:** Internal team (10 users, 2 weeks)
2. **Beta Testing:** TestFlight/Play Console (100 users, 4 weeks)
3. **Public Release:** Staged rollout (10% → 50% → 100%)

### Test Scenarios
- First-time user onboarding flow
- Practice session end-to-end (record, analyze, results)
- Group creation and invitation flow
- Offline mode with sync recovery
- Payment flow for premium subscription
- Accessibility with screen readers
- Performance on minimum spec devices
- Network interruption handling

---

## Appendix: Design Mockups & Wireframes

*(Placeholder for future design assets)*

- Login/Signup screens
- Home dashboard
- Master call library
- Practice session recording interface
- Results screen with coaching feedback
- Group list and detail views
- Leaderboard screens
- Settings menu hierarchy

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-11-08 | AI Assistant | Initial comprehensive user stories document |

---

## Related Documentation

- `MICROSERVICES_ARCHITECTURE_GUIDE.md` - Backend REST API design
- `MICROSERVICES_IMPLEMENTATION_SUMMARY.md` - Implementation roadmap
- `phase3_ai-coach_todo.md` - Learning platform features (Phase 3)
- `docs/architecture.md` - C++ audio engine architecture
- `docs/mvp_todo.md` - Development task tracking

---

**Feedback & Questions:**  
For questions about these user stories, contact the product team or open an issue in the repository.
