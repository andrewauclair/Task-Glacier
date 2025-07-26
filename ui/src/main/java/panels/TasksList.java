package panels;

import config.TaskConfig;
import data.Task;
import data.TaskModel;
import data.TaskState;
import dialogs.AddModifyTask;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DockingRegion;
import io.github.andrewauclair.moderndocking.DynamicDockableParameters;
import io.github.andrewauclair.moderndocking.app.Docking;
import net.byteseek.swing.treetable.TreeTableHeaderRenderer;
import net.byteseek.swing.treetable.TreeTableModel;
import net.byteseek.swing.treetable.TreeUtils;
import packets.*;
import taskglacier.MainFrame;
import tree.TaskTreeTable;
import tree.TaskTreeTableModel;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.tree.*;
import java.awt.*;
import java.awt.datatransfer.StringSelection;
import java.awt.datatransfer.Transferable;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public class TasksList extends JPanel implements Dockable {
    @DockingProperty(name = "taskID", required = true)
    private int taskID = 0;

    @DockingProperty(name = "allTasks", required = true)
    private boolean allTasks = false;

    private String persistentID;
    private String titleText;
    private String tabText;

    private MainFrame mainFrame;

    private TaskTreeTable taskTable;
    public JTextField search = new JTextField(20);

    public TasksList(MainFrame mainFrame) {
        allTasks = true;

        this.mainFrame = mainFrame;
        this.persistentID = "tasks";
        this.titleText = "Tasks";
        this.tabText = "Tasks";

        Docking.registerDockable(this);

        Task rootObject = new Task(0, 0, "");
        configure(rootObject);
    }

    public TasksList(MainFrame mainFrame, Task task) {
        taskID = task.id;
        this.mainFrame = mainFrame;
        persistentID = "task-list-" + task.id;
        titleText = "Tasks (" + task.name + ")";
        tabText = "Tasks (" + task.name + ")";

        Docking.registerDockable(this);

        configure(task);
    }

    public TasksList(DynamicDockableParameters parameters) {
        super(new BorderLayout());

        this.persistentID = parameters.getPersistentID();
        this.titleText = parameters.getTitleText();
        this.tabText = parameters.getTabText();

        mainFrame = MainFrame.mainFrame;

        Docking.registerDockable(this);

        Task rootObject = new Task(0, 0, "");
        configure(rootObject);
    }

    @Override
    public void updateProperties() {
        System.out.println("AltTasksList.updateProperties");
        mainFrame = MainFrame.mainFrame;

        if (allTasks) {
        }
        else {
            Task task = mainFrame.getTaskModel().getTask(taskID);

            if (task != null) {
                configure(task);
            }
            else {
            }
        }
    }

    private void configure(Task rootObject) {
        taskTable = new TaskTreeTable(mainFrame, rootObject, taskID, true);

        // Register the keyboard shortcut
        KeyStroke keyStroke = KeyStroke.getKeyStroke(KeyEvent.VK_F, KeyEvent.CTRL_DOWN_MASK);
        getInputMap(JComponent.WHEN_ANCESTOR_OF_FOCUSED_COMPONENT).put(keyStroke, "search");
        getActionMap().put("search", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                search.requestFocus();
            }
        });

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JScrollPane(taskTable), gbc);
        gbc.gridy++;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        add(search, gbc);
    }

    @Override
    public String getPersistentID() {
        return persistentID;
    }

    @Override
    public String getTabText() {
        return tabText;
    }

    @Override
    public String getTitleText() {
        return titleText;
    }

    @Override
    public boolean isWrappableInScrollpane() {
        return false;
    }
}
