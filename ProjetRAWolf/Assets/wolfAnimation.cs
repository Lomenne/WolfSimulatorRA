using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class wolfAnimation : MonoBehaviour {

    Animator anim;
    public Button run;
    public Button walk;
    public Button idle;
    public Button site;
    public Button creep;

    // Use this for initialization
    void Start () {
        anim = GetComponent<Animator>();
        run.onClick.AddListener(runButtonClick);
        walk.onClick.AddListener(walkButtonClick);
        idle.onClick.AddListener(idleButtonClick);
        site.onClick.AddListener(siteButtonClick);
        creep.onClick.AddListener(creepButtonClick);
    }

    void creepButtonClick()
    {
        anim.Play("03_creep");
    }

    void siteButtonClick()
    {
        anim.Play("05_site");
    }

    void idleButtonClick()
    {
        anim.Play("04_Idle");
    }

    void walkButtonClick()
    {
        anim.Play("02_walk");
    }

    void runButtonClick()
    {
        anim.Play("01_Run");
    }

    // Update is called once per frame
    void Update () {

    }
}
