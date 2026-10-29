#include "ray_cutscene.h"
#include "ray_cutscene_npc_great_dejay_tree.h"

/**
 * @brief Add a cutscene style for Great DeJay Tree
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleGreatDejayTree(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_GREAT_DEEJAY_TREE_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Great DeJay Tree", 1,
                     true, true, true);
}

/**
 * @brief Callback to mark the first dialog as viewed
 */
static void endDialog1(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DJ_TREE_1;
}

/**
 * @brief Callback to mark the second dialog as viewed
 */
static void endDialog2(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DJ_TREE_2;
}

/**
 * @brief Callback to mark the third dialog as viewed
 */
static void endDialog3(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DJ_TREE_3;
}

/**
 * @brief Callback to mark the fourth dialog as viewed
 */
static void endDialog4(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DJ_TREE_4;
}

/**
 * @brief Callback to mark the fifth dialog as viewed
 */
static void endDialog5(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DJ_TREE_5;
}

/**
 * @brief Initialize the cutscene for Great DeJay Tree
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneGreatDejayTree(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_GDJT (CS_STYLE_HYUT + 1)
    addCutsceneStyleGreatDejayTree(cutscene);

#define CS_STYLE_NARRATOR (CS_STYLE_GDJT + 1)
    addCutsceneStyleNarrator(cutscene);

    // clang-format off

    if (!ray->p.i.haveEwiOfTime)
    {
        // Before getting Ewi

        if(!(ray->p.cutsceneViewed & TQS_DJ_TREE_1))
        {
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Great DeJay Tree...", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Oh, there's my little Tomi. What's shakin', my dude?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I... Is there something wrong with me?", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "How do you mean, kiddo?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I mean, all the other fairies found kids immediately, and I...", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Ah... I see.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I just... I don't know if I'm cut out for being an adventurer. What if I never leave these woods?", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Hey... you know, I'VE never left these woods.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Sorry...", false, TOMI_EMBARASSED, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "It's ok, dude. I get it.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I just, I want to be... great, you know? Like you. I know all about the Great Fairy Trials...", false, TOMI_PLEADING, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Oh... they are still talking about that ol' prophecy, huh?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Ever since I can remember, I've wanted to complete the trials, and take my rightful place among the Great Fairies...", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Uhh... huh. I mean, right... But like... are you sure that's what you want?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "What do you mean, \"am I sure?\"", false, TOMI_SUSPICIOUS, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, " Well, like, there are so many other things you could do with your time...", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "... What are you saying?", false, TOMI_SUSPICIOUS, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "I mean, not everyone has to -", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "- You don't think I can do it!", false, TOMI_SHOCKED, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "I didn't-", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "UGH, of COURSE. Nobody here in these stupid woods thinks I can do anything.", false, TOMI_ANGRY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "I -", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "You're just like everyone else. Ugh. I can't believe I came to you for advice.", false, TOMI_ANGRY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Tomi...", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I'll show you - I'll show everyone in these stinkin' woods!", false, TOMI_PLEADING_TEARS, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "T-", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I'm outta here.", false, TOMI_ANGRY, endDialog1);
        }
        else
        {
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I MEAN IT! I'M OUTTA HERE!", false, TOMI_ANGRY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "I heard you the first time.", false, 0, NULL);
            // TODO ACHIEVEMENT UNLOCKED: "I Think You've Made Your Point Now" - show the Great DeJay Tree you're a serious adventurer.
        }
    }
    // After getting Ewi, before visiting Loot
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_INTRO))
    {
        if(!(ray->p.cutsceneViewed & TQS_DJ_TREE_2))
        {
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Check it out, Great DeJay tree! I found a kid after all!", false, TOMI_FIREDUP, NULL);
            addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaa! Hut! Hyup!", false, HYUT_NEUTRAL, NULL);
            addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Hyut somersaults directly into the Great DeJay Tree's trunk.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Hmm.. Well, they certainly seem... spritely.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yep, they sure are!", false, TOMI_EMBARASSED, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "So, I guess we'll be heading off on our adventure now!", false, TOMI_COCKY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "I couldn't stop you if I wanted to.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I mean, you couldn't stop like, anything. You're a tree.", false, TOMI_SUSPICIOUS, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Indeed.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I'll see you again when I'm a Great Fairy!", false, TOMI_COCKY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Tomi... Look, just know... Many others have attempted to complete the Great Fairy Trials. Not everyone found what they're looking for.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, yeah, I know, nobody believes in me, blah blah blah. I get it. We're going!", false, TOMI_BORED, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Ok, Tomi. If you ever want to talk, well - you know where to find me.", false, 0, endDialog2);
        }
        else
        {
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Oh, you want to talk right now?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Oh no, sorry, I was interacting with everything in case I missed something important.", false, TOMI_EMBARASSED, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "That's so thorough of you! Great instinct for an adventurer to have. I'm sure you'll find all sorts of fun things.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Mmmhmm, yep! Definitely wouldn't want to miss any fun conversations!", false, TOMI_EMBARASSED, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Indeed!", false, 0, NULL);
        }
    }
    // After visiting Loot, before getting shield
    else if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_GDJT, "Hello Tomi! I didn't expect to see you back home so soon.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Don't get the wrong idea, you.  I'm just back for my, uh...", false, TOMI_ANGRY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...heart container.", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaaa!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GDJT, "There is nothing wrong with regrouping at home if you need more time, Tomi.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Nope! No no, I'm doing great!", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Just gonna get back out there in that ol' field and make my way through those Great Fairy Trials!", false, TOMI_COCKY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "No need to worry about me! Nope!", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Huup haa!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GDJT, "...", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        if(!(ray->p.cutsceneViewed & TQS_DJ_TREE_3))
        {
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Welcome home, Tomi.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Oh hey... \"Sup.\" Didn't see you there.", false, TOMI_COCKY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "That seems unlikely.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "So, I don't know if you'd heard, but I'm a big deal now. Cleared the whole first level of the Dungeon. Beat a bunch of baddies, got a sacred instrument.", false, TOMI_COCKY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Well done!", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, at this point I'll probably become a Great Fairy in no time. I bet they'll name a speedrunning category after me and everything.", false, TOMI_COCKY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "We're all very impressed, Tomi.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "And I'm sure you're wondering why I'm back home again.", false, TOMI_COCKY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Certainly not to gloat.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Certainly not to gloat.", false, TOMI_COCKY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "...", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "...", false, TOMI_EMBARASSED, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "... Well, if you've got the means now, perhaps you could help out with whatever is causing such a fuss in the Clearing?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I mean, sure, I'm clearly at the stage in my hero's journey where tackling a big intimidating boss is the right next step.", false, TOMI_COCKY, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "You know... whenever you're ready.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yep.", false, TOMI_EMBARASSED, endDialog3);
        }
        else
        {
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Any news about that monster in the Clearing?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "On it, don't worry. Not stalling.", false, TOMI_EMBARASSED, NULL);
        }
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        if(!(ray->p.cutsceneViewed & TQS_DJ_TREE_4))
        {
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Welcome home, Tomi.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hey there.", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Why do you look so glum, sweet pea?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I'm just thinking about something that someone said to me.", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Oh?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Great DeJay Tree - everyone calls you \"Great.\" How come? How did you achieve greatness?", false, TOMI_SUSPICIOUS, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Oh - ha ha, I don't put much stock into labels.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "But really though. Why does everyone here look up to you?", false, TOMI_SUSPICIOUS, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "I mean, they didn't always. For years I was just \"The Tree.\" I was just a little sapling in this little Glen where people sometimes hung out.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Well, it's not like you had anywhere to go.", false, TOMI_SUSPICIOUS, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "True! I watched other adventurers come and go, but I just kinda stayed put. Did my best to be nice to everyone who came though. Sometimes it was sad, feeling like I was missing out on stuff.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I... never really thought about that.", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "But it got better! Then you fairies showed up, and adventurers started coming to the Fairy Glen for companions.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaa!", false, HYUT_NEUTRAL, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "And eventually I was the oldest one here, and people eventually just started calling me \"Great.\" And that's how I became the Great DeJay Tree!", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "So that's your big secret to achieving greatness? Being super old?", false, TOMI_BORED, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Ha ha ha!", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Sorry...", false, TOMI_EMBARASSED, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "You're all good, little sprout. Haven't had a good laugh like that in ages. But hey kiddo, think you could do me a favor?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Sure, what do you need?", false, TOMI_CALM, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "I think I misplaced my DJ TURNTABLES around here somewhere. I can't really look around to check. Mind finding them for me?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Sure thing.", false, TOMI_CALM, endDialog4);
        }
        else
        {
            if(!ray->p.i.haveTurntables)
            {
                addCutsceneLine(cutscene, CS_STYLE_GDJT, "Any luck on finding my TURNTABLES?", false, 0, NULL);
                addCutsceneLine(cutscene, CS_STYLE_TOMI, "No, they've gotta be around here somewhere...", false, TOMI_ANGRY, NULL);
            }
            else
            {
                addCutsceneLine(cutscene, CS_STYLE_GDJT, "Hey sprout! You're back. And you found my TURNTABLES!", false, 0, NULL);
                addCutsceneLine(cutscene, CS_STYLE_TOMI, "Sure do! You ready to spin for us, ya' old fart?", false, TOMI_FIREDUP, NULL);
                addCutsceneLine(cutscene, CS_STYLE_GDJT, "Heh, I'm still learning how to use 'em. I just need another 100 years, and then I'll really have the flow down.", false, 0, NULL);
                addCutsceneLine(cutscene, CS_STYLE_TOMI, "I look forward to hearing it, if I'm still alive then.", false, TOMI_CALM, NULL);
                addCutsceneLine(cutscene, CS_STYLE_GDJT, "Me too. Hey, you can have my student model to practice with if you want.", false, 0, NULL);
                addCutsceneLine(cutscene, CS_STYLE_TOMI, "Really? Wow!", false, TOMI_FIREDUP, NULL);
                addCutsceneLine(cutscene, CS_STYLE_GDJT, "Maybe we'll collab one day?", false, 0, NULL);
                addCutsceneLine(cutscene, CS_STYLE_TOMI, "I'd like that a lot.", false, TOMI_CALM, NULL);
                addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "You Got the Magic Turntables! Activate them by stepping on a DJ pad, and use both touch strips to scratch a fat beat!", false, 0, NULL);
                addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hey, Great DeJay Tree?", false, TOMI_SUSPICIOUS, NULL);
                addCutsceneLine(cutscene, CS_STYLE_GDJT, "Yeah, Tomi?", false, 0, NULL);
                addCutsceneLine(cutscene, CS_STYLE_TOMI, "For what it's worth... I think you're pretty great.", false, TOMI_CALM, NULL);
                addCutsceneLine(cutscene, CS_STYLE_GDJT, "Thanks, kiddo. Same to you.", false, 0, NULL);
                addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Tomi gives the Great DeJay Tree a hug.", false, 0, NULL);
                addCutsceneLine(cutscene, CS_STYLE_GDJT, "Now go get 'em, tiger. <3", false, 0, NULL);
                addCutsceneLine(cutscene, CS_STYLE_TOMI, "I will. <3", false, TOMI_FIREDUP, NULL);
            }
        }
    }
    // After getting lullaby
    else
    {
        if(!(ray->p.cutsceneViewed & TQS_DJ_TREE_5))
        {
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hi, GDJT.", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Oh, \"GDJT\" eh? Rolls right off the tongue, huh.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I'm a little scared.", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "How come, kiddo?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Doria told me I have to sing her lullaby in Gaylorida town square in order to complete the Great Fairy Trials... but I don't know if I can.", false, TOMI_EMBARASSED, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Getting stage fright, little one?", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "It's not that... well, it's not JUST that. I'm just scared.", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "What if I can't complete the Trial?", false, TOMI_PLEADING, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Hmmm...", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Or... what if becoming a Great Fairy doesn't make me happy?", false, TOMI_PLEADING, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Ah, I see. Well, these are good questions. The fact that you are even asking them shows you're a different fairy than the one who stormed out of here not so long ago.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I guess some part of me doesn't want things to change.", false, TOMI_SAD, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Ah... I can certainly relate to that. From my perspective, things are always moving too fast.", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I suppose I could keep exploring and look secrets and stuff before I finish my adventure?", false, TOMI_SUSPICIOUS, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "No objections from me!", false, 0, endDialog5);
        }
        else
        {
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "Do you have any secrets, GDJT?", false, TOMI_SUSPICIOUS, NULL);
            addCutsceneLine(cutscene, CS_STYLE_GDJT, "Nah. But I'll be spinning a set later this weekend if you wanna come by!", false, 0, NULL);
            addCutsceneLine(cutscene, CS_STYLE_TOMI, "I'll... uh... consider it!", false, TOMI_EMBARASSED, NULL);
        }
    }

    // clang-format on
}
