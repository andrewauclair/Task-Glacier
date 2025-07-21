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
import packets.PacketType;
import packets.RequestID;
import packets.TaskStateChange;
import packets.UpdateTask;
import taskglacier.MainFrame;

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
import java.util.Objects;

public class TasksList extends JPanel implements Dockable, TaskModel.Listener {
    @DockingProperty(name = "taskID", required = true)
    private int taskID = 0;

    @DockingProperty(name = "allTasks", required = true)
    private boolean allTasks = false;

    private String persistentID;
    private String titleText;
    private String tabText;

    private MainFrame mainFrame;
    private DefaultMutableTreeNode rootNode;
    private TreeTableModel treeTableModel;
    private DefaultTreeModel treeModel;

    private JTable table;
    public JTextField search = new JTextField(20);

    private void finishActiveTask() {
        TaskStateChange change = new TaskStateChange();
        change.packetType = PacketType.FINISH_TASK;
        change.taskID = mainFrame.getTaskModel().getActiveTaskID().get();
        mainFrame.getConnection().sendPacket(change);
    }

    private void changeTaskState(PacketType type) {
        int selectedRow = table.getSelectedRow();

        if (selectedRow == -1) {
            return;
        }

        Task task = (Task) ((DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(selectedRow)).getUserObject();

        TaskStateChange change = new TaskStateChange();
        change.packetType = type;
        change.taskID = task.id;
        mainFrame.getConnection().sendPacket(change);
    }

    public TasksList(MainFrame mainFrame) {
        allTasks = true;

        this.mainFrame = mainFrame;
        this.persistentID = "tasks";
        this.titleText = "Tasks";
        this.tabText = "Tasks";

        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);

        Task rootObject = new Task(0, 0, "");
        rootNode = TreeUtils.buildTree(rootObject, Task::getChildren, parent -> false);

        configure();
    }

    public TasksList(MainFrame mainFrame, Task task) {
        taskID = task.id;
        this.mainFrame = mainFrame;
        persistentID = "task-list-" + task.id;
        titleText = "Tasks (" + task.name + ")";
        tabText = "Tasks (" + task.name + ")";

        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);

        rootNode = TreeUtils.buildTree(task, Task::getChildren, parent -> false);

        configure();

        addTasks(task);

        treeTableModel.expandChildren(rootNode);
    }

    public TasksList(DynamicDockableParameters parameters) {
        super(new BorderLayout());

        this.persistentID = parameters.getPersistentID();
        this.titleText = parameters.getTitleText();
        this.tabText = parameters.getTabText();

        mainFrame = MainFrame.mainFrame;

        Docking.registerDockable(this);
        mainFrame.getTaskModel().addListener(this);

        Task rootObject = new Task(0, 0, "");
        rootNode = TreeUtils.buildTree(rootObject, Task::getChildren, parent -> false);

        configure();
    }

    @Override
    public void updateProperties() {
        System.out.println("AltTasksList.updateProperties");
        mainFrame = MainFrame.mainFrame;

        if (allTasks) {
//            table.expandAll();
//            Task rootObject = new Task(0, 0, "");
//            rootNode = TreeUtils.buildTree(rootObject, Task::getChildren, parent -> false);
//            treeTableModel.setRoot(rootNode);
//            treeModel.setRoot(rootNode);
        }
        else {
            Task task = mainFrame.getTaskModel().getTask(taskID);

            if (task != null) {
//                table.setRootVisible(false);
//                treeTableModel.setRoot(new ParentTaskTreeTableNode(task));
//                addTasks(task);
                rootNode = TreeUtils.buildTree(task, Task::getChildren, parent -> false);
                treeTableModel.setRoot(rootNode);
                treeModel.setRoot(rootNode);

                addTasks(task);
            }
            else {
//                Task rootObject = new Task(0, 0, "");
//                rootNode = TreeUtils.buildTree(rootObject, Task::getChildren, parent -> false);
//                treeTableModel.setRoot(rootNode);
//                treeModel.setRoot(rootNode);
            }
//            table.expandAll();
        }
//        treeTableModel.expandTree();
    }

    private void addTasks(Task task) {
        for (Task child : task.children) {
            newTask(child);
            addTasks(child);
        }
    }

    private void configure() {
        table = new JTable();
        table.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
        table.setDragEnabled(true);
        table.setDropMode(DropMode.ON_OR_INSERT_ROWS);
        table.setTransferHandler(new TaskTransferHandler());
        treeTableModel = createTreeTableModel(rootNode);
        treeModel = createTreeModel(rootNode);

        DefaultTreeCellRenderer  treeCellRenderer = new DefaultTreeCellRenderer () {
            @Override
            public Component getTreeCellRendererComponent(JTree tree, Object value, boolean selected, boolean expanded, boolean leaf, int row, boolean hasFocus) {
                Component c = super.getTreeCellRendererComponent(tree, value, selected, expanded, leaf, row, hasFocus);

                TaskTreeTableNode node = (TaskTreeTableNode) value;

                Task task = (Task) node.getUserObject();

                if (task != null) {
                    setText(task.name);
                }

                if (node.isActiveTask()) {
                    setIcon(new ImageIcon(Objects.requireNonNull(getClass().getResource("/active.png"))));
                }
                else if (node.hasActiveChildTask()) {
                    setIcon(new ImageIcon(Objects.requireNonNull(getClass().getResource("/activeChild.png"))));
                }

                return c;
            }
        };


        search.getDocument().addDocumentListener(new DocumentListener() {
            @Override
            public void insertUpdate(DocumentEvent e) {
                updateFilter();
            }

            @Override
            public void removeUpdate(DocumentEvent e) {
                updateFilter();
            }

            @Override
            public void changedUpdate(DocumentEvent e) {
                updateFilter();
            }
        });

        // Register the keyboard shortcut
        KeyStroke keyStroke = KeyStroke.getKeyStroke(KeyEvent.VK_F, KeyEvent.CTRL_DOWN_MASK);
        getInputMap(JComponent.WHEN_ANCESTOR_OF_FOCUSED_COMPONENT).put(keyStroke, "search");
        getActionMap().put("search", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                search.requestFocus();
            }
        });

        treeTableModel.setNodeFilter(treeNode -> {
            Task obj = TreeUtils.getUserObject(treeNode);
            return obj.state == TaskState.FINISHED;
        });

        JMenuItem add = new JMenuItem("Add Task...");
        JMenuItem addSubTask = new JMenuItem("Add Sub-Task...");
        JMenuItem start = new JMenuItem("Start");
        JMenuItem startStopActive = new JMenuItem("Start (Stop Active)");
        JMenuItem startFinishActive = new JMenuItem("Start (Finish Active)");
        JMenuItem stop = new JMenuItem("Stop");
        JMenuItem finish = new JMenuItem("Finish");
        JMenuItem openInNewWindow = new JMenuItem("Open in New List");
        JMenuItem config = new JMenuItem("Configure...");

        config.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            Task task = (Task) ((DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(selectedRow)).getUserObject();

            TaskConfig dialog = new TaskConfig(mainFrame, mainFrame, task);
            dialog.setVisible(true);
        });

        start.addActionListener(e -> changeTaskState(PacketType.START_TASK));
        startStopActive.addActionListener(e -> changeTaskState(PacketType.START_TASK));
        startFinishActive.addActionListener(e -> {
            finishActiveTask();
            changeTaskState(PacketType.START_TASK);
        });
        stop.addActionListener(e -> changeTaskState(PacketType.STOP_TASK));
        finish.addActionListener(e -> changeTaskState(PacketType.FINISH_TASK));

        add.addActionListener(e -> new AddModifyTask(mainFrame, mainFrame, 0, false).setVisible(true));

        addSubTask.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            Task task = (Task) ((DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(selectedRow)).getUserObject();

            new AddModifyTask(mainFrame, mainFrame, task.id, false).setVisible(true);
        });

        openInNewWindow.addActionListener(e -> {
            int selectedRow = table.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            Task task = (Task) ((DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(selectedRow)).getUserObject();

            if (!Docking.isDockableRegistered("tasks-list-" + task.id)) {
                TasksList newList = new TasksList(mainFrame, task);
                Docking.dock(newList, TasksList.this, DockingRegion.CENTER);
            }
            else {
                Docking.display("tasks-list-" + task.id);
            }
        });

        table.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    int selectedRow = table.getSelectedRow();

                    JPopupMenu contextMenu = new JPopupMenu();

                    if (selectedRow == -1) {
                        contextMenu.add(add);
                        contextMenu.show(table, e.getX(), e.getY());
                        return;
                    }

                    contextMenu.add(config);

                    if (mainFrame.getTaskModel().getActiveTaskID().isPresent() &&
                            !mainFrame.getTaskModel().taskHasNonFinishedChildren(mainFrame.getTaskModel().getActiveTaskID().get()) &&
                            !mainFrame.getTaskModel().getTask(mainFrame.getTaskModel().getActiveTaskID().get()).locked) {
                        contextMenu.add(startStopActive);
                        contextMenu.add(stop);
                        contextMenu.add(startFinishActive);
                        contextMenu.add(finish);
                    }
                    else {
                        contextMenu.add(start);
                        contextMenu.add(stop);
                        contextMenu.add(finish);
                    }


                    contextMenu.addSeparator();
                    contextMenu.add(addSubTask);

                    TreeNode node = treeTableModel.getNodeAtTableRow(selectedRow);
                    Task task = (Task) ((DefaultMutableTreeNode) node).getUserObject();

                    startStopActive.setEnabled(task.state == TaskState.PENDING);
                    startFinishActive.setEnabled(task.state == TaskState.PENDING);
                    stop.setEnabled(task.state == TaskState.ACTIVE);
                    finish.setEnabled(task.state != TaskState.FINISHED && !mainFrame.getTaskModel().taskHasNonFinishedChildren(task.id) && !task.locked && !task.serverControlled);

                    // task has subtasks, allow an option to open it in a new panel
                    if (node.getChildCount() != 0) {
                        contextMenu.addSeparator();
                        contextMenu.add(openInNewWindow);
                    }

                    contextMenu.show(table, e.getX(), e.getY());
                }
                else if (e.getClickCount() == 2 && SwingUtilities.isLeftMouseButton(e)) {
                    config.doClick();
                }
            }
        });

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JScrollPane(table), gbc);
        gbc.gridy++;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        add(search, gbc);
    }

    private void updateFilter() {
        if (search.getText().isEmpty()) {
            treeTableModel.setNodeFilter(treeNode -> {
                Task obj = TreeUtils.getUserObject(treeNode);
                return obj.state == TaskState.FINISHED;
            });
        } else {
            treeTableModel.setNodeFilter(treeNode -> {
                Task obj = TreeUtils.getUserObject(treeNode);
                boolean includeFinish = search.getText().startsWith("finish: ");
                String text = search.getText();
                if (includeFinish) {
                    text = text.substring("finish: ".length());
                    return !childrenHaveMatch(obj, text);
                }
                return obj.state == TaskState.FINISHED || !childrenHaveMatch(obj, text);
            });
        }
    }

    class TaskTransferHandler extends TransferHandler {
        private List<Integer> rows = new ArrayList<>();

        @Override
        public int getSourceActions(JComponent c) {
            return MOVE;
        }

        @Override
        protected Transferable createTransferable(JComponent c) {
            int[] selectedRows = table.getSelectedRows();

            rows = new ArrayList<>();
            
            for (int i = selectedRows.length - 1; i >= 0; i--) {
                DefaultMutableTreeNode nodeAtTableRow = (DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(selectedRows[i]);
                Task task = (Task) nodeAtTableRow.getUserObject();

                if (!task.serverControlled && !task.locked) {
                    rows.add(selectedRows[i]);
                }
            }

            if (rows.isEmpty()) {
                return null;
            }
            return new StringSelection("");
        }

        @Override
        public boolean canImport(TransferSupport support) {
            return true;
        }

        @Override
        public boolean importData(TransferSupport support) {
            // send to server
            for (int row : rows) {
                DefaultMutableTreeNode nodeAtTableRow = (DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(row);
                Task task = (Task) nodeAtTableRow.getUserObject();

                JTable.DropLocation dl = (JTable.DropLocation) support.getDropLocation();

                DefaultMutableTreeNode dropNode = (DefaultMutableTreeNode) treeTableModel.getNodeAtTableRow(dl.getRow());

                // dropping after the last node on the tree
                if (dropNode == null) {
                    System.out.println("insert as last task in root list");


                }
                else {
                    Task dropTask = (Task) dropNode.getUserObject();
                    Task parentTask;

                    // the drop row is always the row below where the insert line is
                    if (dl.isInsertRow()) {
                        parentTask = mainFrame.getTaskModel().getTask(dropTask.parentID);
                    }
                    else {
                        parentTask = dropTask;
                    }
                    int index = dl.isInsertRow() ? dropTask.indexInParent : 0;

                    if (parentTask == null) {
                        System.out.println("insert with root as parent");

                        UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task.id, 0, task.name);
                        update.indexInParent = index;
                        mainFrame.getConnection().sendPacket(update);
                    }
                    else {
                        System.out.println("Reparent '" + task.name + "' to '" + parentTask.name);

                        UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task.id, parentTask.id, task.name);
                        update.indexInParent = index;
                        mainFrame.getConnection().sendPacket(update);
                    }
                    // move task: request id, task id, new parent id

                }
            }
            return true;
        }
    }

    private boolean childrenHaveMatch(Task obj, String text) {
        for (Task child : obj.children) {
            if (childrenHaveMatch(child, text)) {
                return true;
            }
        }
        if (obj.name.toLowerCase().contains(text.toLowerCase())) {
            return true;
        }
        return false;
    }

    private DefaultTreeModel createTreeModel(TreeNode rootNode) {
        DefaultTreeModel model = new DefaultTreeModel(rootNode);
        model.addTreeModelListener(treeTableModel);
        return model;
    }

    private TreeTableModel createTreeTableModel(TreeNode rootNode) {
        TreeTableModel localTreeTableModel = new TaskTreeTableModel(rootNode, false);

        TreeTableHeaderRenderer renderer = new TreeTableHeaderRenderer();
        renderer.setShowNumber(true); // true is default, this code is just for testing the false option.

        localTreeTableModel.bindTable(table, renderer); //, new RowSorter.SortKey(0, SortOrder.ASCENDING));
        localTreeTableModel.addExpandCollapseListener(new TreeTableModel.ExpandCollapseListener() {
            @Override
            public boolean nodeExpanding(TreeNode node) {
                if (node.getChildCount() == 0) { // if a node is expanding but has no children, set it to allow no children.
                    ((DefaultMutableTreeNode) node).setAllowsChildren(false);
                }
                return true;
            }

            @Override
            public boolean nodeCollapsing(TreeNode node) {
                return true;
            }
        });
        return localTreeTableModel;
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

    @Override
    public void newTask(Task task) {
        System.out.println("AltTasksList.newTask");
        DefaultMutableTreeNode parent = findTaskNode(rootNode, task.parentID);
        if (parent == null) {
            int breakpoint = 0;
            return;
        }
        MutableTreeNode newChild = new DefaultMutableTreeNode(task);

        parent.setAllowsChildren(true);
        parent.add(newChild);
        treeTableModel.treeNodeInserted(parent, parent.getChildCount() - 1);

        if (task.parentID == 0 && parent.getChildCount() == 1) {
            treeTableModel.expandTree();
        }
    }

    @Override
    public void updatedTask(Task task) {
        System.out.println("AltTasksList.updatedTask");
        DefaultMutableTreeNode node = findTaskNode(rootNode, task.id);

        if (node != null) {
            treeTableModel.treeNodeChanged(node);

            if (task.state == TaskState.FINISHED) {
                updateFilter();

                Task parent = mainFrame.getTaskModel().getTask(task.parentID);
                DefaultMutableTreeNode parentNode = findTaskNode(rootNode, task.parentID);

                if (parent != null && parentNode != null) {
                    boolean active = parent.children.stream()
                            .anyMatch(task1 -> task1.state != TaskState.FINISHED);

                    parentNode.setAllowsChildren(active);
                }
            }
        }
    }

    @Override
    public void reparentTask(Task task, int oldParent) {
        System.out.println("AltTasksList.reparentTask");
        DefaultMutableTreeNode oldParentNode = findTaskNode(rootNode, oldParent);
        DefaultMutableTreeNode newParentNode = findTaskNode(rootNode, task.parentID);

        DefaultMutableTreeNode node = findTaskNode(rootNode, task.id);

        if (node == null) {
            return;
        }

        if (oldParentNode != null) {
            if (oldParentNode.getChildCount() == 0) {
                oldParentNode.setAllowsChildren(false);
            }
            // also removes the node from the old parent for us
            treeTableModel.treeNodeRemoved(oldParentNode, node);
        }

        if (newParentNode != null) {
            newParentNode.setAllowsChildren(true);
            newParentNode.add(node);
            treeTableModel.treeNodeInserted(newParentNode, newParentNode.getChildCount() - 1);
        }
    }

    @Override
    public void configComplete() {
        treeTableModel.expandTree();
    }

    private DefaultMutableTreeNode findTaskNode(DefaultMutableTreeNode currentParent, int parentID) {
        if (parentID == taskID) {
            return rootNode;
        }
        Enumeration<TreeNode> children = currentParent.children();

        while (children.hasMoreElements()) {
            DefaultMutableTreeNode node = (DefaultMutableTreeNode) children.nextElement();

            Task task = (Task) node.getUserObject();

            if (task.id == parentID) {
                return node;
            }

            if (node.getChildCount() != 0) {
                DefaultMutableTreeNode result = findTaskNode(node, parentID);

                if (result != null) {
                    return result;
                }
            }
        }

        return null;
    }
}
